# GP_battery - general purpose battery class
# Copyright (C) 2021 Dave Gutz
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation;
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# See http://www.fsf.org/licensing/licenses/lgpl.txt for full license text.

"""Define a general purpose battery model including Randle's model and SoC-VOV model as well as Kalman filtering
support for simplified Mathworks' tracker. See Huria, Ceraolo, Gazzarri, & Jackey, 2013 Simplified Extended Kalman
Filter Observer for SOC Estimation of Commercial Power-Oriented LFP Lithium Battery Cells """

import numpy as np
import math
from pyDAGx.lookup_table import LookupTable


class Battery:
    # Battery model:  Randle's dynamics, SOC-VOC model

    def __init__(self, n_cells=4, r0=0.003, tau_ct=3.7, rct=0.0016, tau_dif=83, r_dif=0.0077, dt=0.1, b=0., a=0.,
                 c=0., n=0.4, m=0.478, d=0.707, t_t=None, t_b=None, t_a=None, t_c=None, nom_bat_cap=100.,
                 true_bat_cap=102., nom_sys_volt=13., dv=0, sr=1, bat_v_sat=3.4625, dvoc_dt=0.001875):
        """ Default values from Taborelli & Onori, 2013, State of Charge Estimation Using Extended Kalman Filters for
        Battery Management System.   Battery equations from LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang
        etal.pdf.'  SOC-OCV curve fit './Battery State/BattleBorn Rev1.xls:Model Fit' using solver with min slope
        constraint >=0.02 V/soc.  m and n using Zhang values.   Had to scale soc because  actual capacity > NOM_BAT_CAP
        so equations error when soc<=0 to match data.
        """
        # Defaults for mutable arguments
        if t_t is None:
            t_t = [0., 25., 50.]
        if t_b is None:
            t_b = [-0.836, -0.836, -0.836]
        if t_a is None:
            t_a = [3.999, 4.046, 4.093]
        if t_c is None:
            t_c = [-1.181, -1.181, -1.181]
        # Dynamics
        self.n_cells = n_cells
        self.r_0 = r0 * n_cells
        self.tau_ct = tau_ct
        self.r_ct = rct * n_cells
        self.c_ct = self.tau_ct / self.r_ct
        self.tau_dif = tau_dif
        self.r_dif = r_dif * n_cells
        self.c_dif = self.tau_dif / self.r_dif
        self.A, self.B, self.C, self.D = self.construct_state_space()
        self.u = np.array([0., 0]).T
        self.dt = dt
        self.x = None
        self.x_past = None
        self.x_dot = self.x
        self.y = np.array([0., 0]).T

        # SOC-VOC
        # Battery coefficients
        self.b = b
        self.a = a
        self.c = c
        self.d = d
        self.n = n
        self.m = m
        self.lut_b = LookupTable()
        self.lut_a = LookupTable()
        self.lut_c = LookupTable()
        self.lut_b.addAxis('T_degC', t_t)
        self.lut_a.addAxis('T_degC', t_t)
        self.lut_c.addAxis('T_degC', t_t)
        self.lut_b.setValueTable(t_b)
        self.lut_a.setValueTable(t_a)
        self.lut_c.setValueTable(t_c)

        # Other things
        self.soc = 0.  # State of charge, %
        self.soc_norm = 0.  # State of charge, normalized, %
        self.dV_dSoc = 0.  # Slope of soc-voc curve, V/%
        self.dV_dSoc_norm = 0.  # Slope of soc-voc curve, normalized, V/%
        self.nom_bat_cap = nom_bat_cap
        """ Nominal battery label capacity, e.g. 100, Ah. Accounts for internal losses.
            This is what gets delivered, e.g. W_shunt/NOM_SYS_VOLT.  Also varies 0.2-0.4C
            currents or 20-40 A for a 100 Ah battery"""
        self.nom_sys_volt = nom_sys_volt  # Nominal battery label voltage, e.g. 12, V
        self.v_sat = nom_sys_volt + 1.7  # Saturation voltage, V
        self.voc = nom_sys_volt  # Nominal battery storage voltage, V
        self.sat = True  # Battery is saturated, T/F
        self.ib = 0  # Current into battery post, A
        self.vb = nom_sys_volt  # Battery voltage at post, V
        self.ioc = 0  # Current into battery storage, A
        self.cu = nom_bat_cap  # Assumed capacity, Ah
        self.cs = true_bat_cap  # Data fit to this capacity to avoid math 0, Ah
        self.dv = dv  # Adjustment for voltage level, V (0)
        self.sr = sr  # Adjustment for resistance scalar, fraction (1)
        self.pow_in = None  # Charging power, W
        self.time_charge = None  # Charging time to 100%, hr
        self.nom_v_sat = bat_v_sat * self.n_cells  # Normal battery cell saturation for SOC=99.7, V (3.4625 = 13.85v)
        self.dVoc_dT = dvoc_dt * self.n_cells  # Change of VOC with operating temperature in range 0 - 50 C, V/deg C
        self.eps_max = 1 - 1e-6  # Numerical maximum of coefficient model with scaled soc_norm.
        self.eps_min = 1e-6  # Numerical minimum of coefficient model without scaled soc_norm.
        # eps_min_unscaled = 1 - (
        #       1 - eps_min) * self.cs / self.cu  # Numerical minimum of coefficient model without scaled soc_norm.

    def calc_voc(self, temp_c=25., soc_init=0.5):
        # SOC-OCV curve fit method per Zhang, et al
        # Inputs:  soc_norm
        # Outputs: voc  + various other
        # Initialize
        if self.pow_in is None:
            self.soc = soc_init
            self.soc_norm = 1. - (1. - self.soc) * self.cu / self.cs
        self.b, self.a, self.c = self.look(temp_c)

        # Perform computationally expensive steps one time
        soc_norm_lim = max(min(self.soc_norm, self.eps_max), self.eps_min)
        log_soc_norm = math.log(soc_norm_lim)
        exp_n_soc_norm = math.exp(self.n * (soc_norm_lim - 1))
        pow_log_soc_norm = math.pow(-log_soc_norm, self.m)

        # SOC-OCV model
        self.dV_dSoc_norm = float(self.n_cells) * (self.b * self.m / soc_norm_lim * pow_log_soc_norm / log_soc_norm +
                                                   self.c + self.d * self.n * exp_n_soc_norm)
        self.dV_dSoc = self.dV_dSoc_norm * self.cu / self.cs
        # slightly beyond
        self.voc = float(self.n_cells) * (self.a + self.b * pow_log_soc_norm +
                                          self.c * soc_norm_lim + self.d * exp_n_soc_norm) + (
                               self.soc_norm - soc_norm_lim) * self.dV_dSoc_norm
        self.voc += self.dv  # Experimentally varied
        # self.d2V_dSocs2 = float(self.n_cells) * ( self.b*self.m/self.soc/self.soc*pow_log_soc_norm/
        # log_soc_norm*((self.m-1.)/log_soc_norm - 1.) + self.d*self.n*self.n*exp_n_soc_norm )
        self.v_sat = self.nom_v_sat + (temp_c - 25.) * self.dVoc_dT
        self.sat = self.voc >= self.v_sat
        self.u[1] = self.voc

    def calc_dynamics(self, u, dt=None):
        # Executive model dynamics
        if dt is not None:
            self.dt = dt
        self.u = u

        # Dynamics
        # State-space calculation
        # Inputs: [ib, voc].T
        # Outputs:[vb, ioc].T
        self.propagate_state_space(self.u, dt=dt)

        # Coulomb counter
        self.coulomb_counter()

        # Summarize
        soc_norm_lim = max(min(self.soc_norm, self.eps_max), self.eps_min)
        if self.pow_in > 1.:
            # nom_bat_cap is defined at nom_sys_volt
            self.time_charge = min(self.nom_bat_cap / self.pow_in * self.nom_sys_volt * (1. - soc_norm_lim), 24.)
        elif self.pow_in < -1.:
            # nom_bat_cap is defined at nom_sys_volt
            self.time_charge = max(self.nom_bat_cap / self.pow_in * self.nom_sys_volt * soc_norm_lim, -24.)
        elif self.pow_in >= 0.:
            self.time_charge = 24. * (1. - soc_norm_lim)
        else:
            self.time_charge = -24. * soc_norm_lim

        return self.vb, self.dV_dSoc_norm

    def construct_state_space(self):
        """ State-space representation of dynamics
        Inputs:   ib - current at V_bat = Vb = current at shunt, A
                  voc - internal open circuit voltage, V
        Outputs:  vb - voltage at positive, V
                  ioc  - current into storage, A
        States:   vbc - RC vb-vc, V
                  vcd - RC vc-vd, V
        Other:    vc - voltage downstream of charge transfer model, ct-->c
                  vd - voltage downstream of diffusion process model, dif-->d
        """
        a = np.array([[-1 / self.tau_ct, 0],
                      [0, -1 / self.tau_dif]])
        b = np.array([[1 / self.c_ct, 0],
                      [1 / self.c_dif, 0]])
        c = np.array([[1, 1],
                      [0, 0]])
        d = np.array([[self.r_0, 1],
                      [1, 0]])
        return a, b, c, d

    def coulomb_counter(self):
        # Coulomb counter
        # Inputs: ib, vb, ioc, i_r_dif, i_r_ct
        # Outputs:  soc, soc_norm
        # Internal resistance of battery is a loss
        self.pow_in = self.ioc * self.voc * self.sr
        self.soc = max(min(self.soc + self.pow_in / self.nom_sys_volt * self.dt / 3600. / self.nom_bat_cap, 1.5), 0.)
        if self.sat:
            self.soc = self.eps_max
        self.soc_norm = 1. - (1. - self.soc) * self.cu / self.cs

    def propagate_state_space(self, u, dt=None):
        # Time propagation from state-space using backward Euler
        # Inputs: [ib, voc].T
        # Outputs:[vb, ioc].T
        self.u = u
        self.ib = self.u[0]
        self.voc = self.u[1]
        if self.x is None:  # Initialize
            self.x = np.array([self.ib * self.r_ct, self.ib * self.r_dif]).T
        if dt is not None:
            self.dt = dt
        self.x_past = self.x
        self.x_dot = self.A @ self.x + self.B @ self.u
        self.x += self.x_dot * self.dt
        self.y = self.C @ self.x_past + self.D @ self.u  # uses past (backward Euler)
        self.vb = self.y[0]
        self.ioc = self.y[1]

    def look(self, temp_c):
        b = self.lut_b.lookup(T_degC=temp_c)
        a = self.lut_a.lookup(T_degC=temp_c)
        c = self.lut_c.lookup(T_degC=temp_c)
        return b, a, c

    def vbc(self):
        return self.x[0]

    def vcd(self):
        return self.x[1]

    def vbc_dot(self):
        return self.x_dot[0]

    def vcd_dot(self):
        return self.x_dot[1]

    def i_c_ct(self):
        return self.vbc_dot() * self.c_ct

    def i_c_dif(self):
        return self.vcd_dot() * self.c_dif

    def i_r_ct(self):
        return self.vbc() / self.r_ct

    def i_r_dif(self):
        return self.vcd() / self.r_dif

    def vd(self):
        return self.voc + self.ioc * self.r_0

    def vc(self):
        return self.vd() + self.vcd()


class BatteryEKF:
    # Battery model for embedded use:  Randle's dynamics, SOC-VOC model, EKF support

    def __init__(self, n_cells=4, r0=0.003, tau_ct=3.7, rct=0.0016, tau_dif=83, r_dif=0.0077, dt=0.1, b=0., a=0.,
                 c=0., n=0.4, m=0.478, d=0.707, t_t=None, t_b=None, t_a=None, t_c=None, nom_bat_cap=100.,
                 true_bat_cap=102., nom_sys_volt=13., dv=0, sr=1, bat_v_sat=3.4625, dvoc_dt=0.001875, rsd=70.,
                 tau_sd=1.8e7):
        """ Default values from Taborelli & Onori, 2013, State of Charge Estimation Using Extended Kalman Filters for
        Battery Management System.   Battery equations from LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang
        etal.pdf.'  SOC-OCV curve fit './Battery State/BattleBorn Rev1.xls:Model Fit' using solver with min slope
        constraint >=0.02 V/soc.  m and n using Zhang values.   Had to scale soc because  actual capacity > NOM_BAT_CAP
        so equations error when soc<=0 to match data.
        """
        # Defaults for mutable arguments
        if t_t is None:
            t_t = [0., 25., 50.]
        if t_b is None:
            t_b = [-0.836, -0.836, -0.836]
        if t_a is None:
            t_a = [3.999, 4.046, 4.093]
        if t_c is None:
            t_c = [-1.181, -1.181, -1.181]
        # Dynamics
        self.n_cells = n_cells
        self.r_0 = r0 * n_cells
        self.tau_ct = tau_ct
        self.r_ct = rct * n_cells
        self.c_ct = self.tau_ct / self.r_ct
        self.tau_dif = tau_dif
        self.r_dif = r_dif * n_cells
        self.c_dif = self.tau_dif / self.r_dif
        self.A, self.B, self.C, self.D = self.construct_state_space_ekf()
        self.u = np.array([0., 0]).T
        self.dt = dt
        self.x = None
        self.x_past = None
        self.x_dot = self.x
        self.y = np.array([0., 0]).T

        # SOC-VOC
        # Battery coefficients
        self.b = b
        self.a = a
        self.c = c
        self.d = d
        self.n = n
        self.m = m
        self.lut_b = LookupTable()
        self.lut_a = LookupTable()
        self.lut_c = LookupTable()
        self.lut_b.addAxis('T_degC', t_t)
        self.lut_a.addAxis('T_degC', t_t)
        self.lut_c.addAxis('T_degC', t_t)
        self.lut_b.setValueTable(t_b)
        self.lut_a.setValueTable(t_a)
        self.lut_c.setValueTable(t_c)

        # Estimator
        # Nominal rsd is loss of 70% charge (Dsoc=0.7) in 6 month (Dt=1.8e7 sec = tau_sd). Dsoc= i*Dt/(3600*nom_bat_cap)
        # R=(nom_bat_volt=1=soc_max)/i = 1/Dsoc*Dt/3600/nom_bat_cap = 70 for 1v, 100 A-h.  Cq=257000 F.
        self.r_sd = rsd
        self.tau_sd = tau_sd
        self.cq = self.tau_sd / self.r_sd

        # Other things
        self.soc = 0.  # State of charge, %
        self.soc_norm = 0.  # State of charge, normalized, %
        self.dV_dSoc = 0.  # Slope of soc-voc curve, V/%
        self.dV_dSoc_norm = 0.  # Slope of soc-voc curve, normalized, V/%
        self.nom_bat_cap = nom_bat_cap
        """ Nominal battery label capacity, e.g. 100, Ah. Accounts for internal losses.
            This is what gets delivered, e.g. W_shunt/NOM_SYS_VOLT.  Also varies 0.2-0.4C
            currents or 20-40 A for a 100 Ah battery"""
        self.nom_sys_volt = nom_sys_volt  # Nominal battery label voltage, e.g. 12, V
        self.v_sat = nom_sys_volt + 1.7  # Saturation voltage, V
        self.voc = nom_sys_volt  # Nominal battery storage voltage, V
        self.sat = True  # Battery is saturated, T/F
        self.ib = 0  # Current into battery post, A
        self.vb = nom_sys_volt  # Battery voltage at post, V
        self.ioc = 0  # Current into battery storage, A
        self.cu = nom_bat_cap  # Assumed capacity, Ah
        self.cs = true_bat_cap  # Data fit to this capacity to avoid math 0, Ah
        self.dv = dv  # Adjustment for voltage level, V (0)
        self.sr = sr  # Adjustment for resistance scalar, fraction (1)
        self.pow_in = None  # Charging power, W
        self.time_charge = None  # Charging time to 100%, hr
        self.nom_v_sat = bat_v_sat * self.n_cells  # Normal battery cell saturation for SOC=99.7, V (3.4625 = 13.85v)
        self.dVoc_dT = dvoc_dt * self.n_cells  # Change of VOC with operating temperature in range 0 - 50 C, V/deg C
        self.eps_max = 1 - 1e-6  # Numerical maximum of coefficient model with scaled soc_norm.
        self.eps_min = 1e-6  # Numerical minimum of coefficient model without scaled soc_norm.
        # eps_min_unscaled = 1 - (
        #       1 - eps_min) * self.cs / self.cu  # Numerical minimum of coefficient model without scaled soc_norm.

    def calc_dynamics_ekf(self, u, dt=None):
        # Executive model dynamics for ekf
        if dt is not None:
            self.dt = dt
        # State-space calculation
        # Inputs: [ib, vb].T
        # Outputs: voc
        self.u = u
        self.propagate_state_space_ekf(self.u, dt=dt)

    def calc_voc_ekf(self, temp_c=25., soc_init=0.5):
        # SOC-OCV curve fit method per Zhang, et al
        # Inputs:  soc_norm
        # Outputs: voc  + various other

        # Initialize
        if self.pow_in is None:
            self.soc = soc_init
            self.soc_norm = 1. - (1. - self.soc) * self.cu / self.cs

        # Tables
        self.b, self.a, self.c = self.look(temp_c)

        # Perform computationally expensive steps one time
        soc_norm_lim = max(min(self.soc_norm, self.eps_max), self.eps_min)
        log_soc_norm = math.log(soc_norm_lim)
        exp_n_soc_norm = math.exp(self.n * (soc_norm_lim - 1))
        pow_log_soc_norm = math.pow(-log_soc_norm, self.m)

        # SOC-OCV model
        # slightly beyond
        self.voc = float(self.n_cells) * (self.a + self.b * pow_log_soc_norm +
                                          self.c * soc_norm_lim + self.d * exp_n_soc_norm) + (
                               self.soc_norm - soc_norm_lim) * self.dV_dSoc_norm
        self.voc += self.dv  # Experimentally varied

    def construct_state_space_ekf(self):
        """ State-space representation of dynamics
        Inputs:   ib - current at V_bat = Vb = current at shunt, A
                  voc - internal open circuit voltage, V
        Outputs:  vb - voltage at positive, V
                  ioc  - current into storage, A
        States:   vbc - RC vb-vc, V
                  vcd - RC vc-vd, V
        Other:    vc - voltage downstream of charge transfer model, ct-->c
                  vd - voltage downstream of diffusion process model, dif-->d
        """
        a = np.array([[-1 / self.tau_ct, 0],
                      [0, -1 / self.tau_dif]])
        b = np.array([[1 / self.c_ct, 0],
                      [1 / self.c_dif, 0]])
        c = np.array([-1, -1])
        d = np.array([-self.r_0, 1])
        return a, b, c, d

    def coulomb_counter_ekf(self, temp_c=25):
        # Coulomb counter
        # Inputs: ib, vb, ioc, i_r_dif, i_r_ct
        # Outputs:  soc, soc_norm
        # Internal resistance of battery is a loss
        self.pow_in = self.ib * (self.vb - (self.ioc * self.r_0 + self.i_r_dif() * self.r_dif +
                                            self.i_r_ct() * self.r_ct) * self.sr)
        self.soc = max(min(self.soc + self.pow_in / self.nom_sys_volt * self.dt / 3600. / self.nom_bat_cap, 1.5), 0.)
        if self.sat:
            self.soc = self.eps_max
        self.soc_norm = 1. - (1. - self.soc) * self.cu / self.cs
        self.v_sat = self.nom_v_sat + (temp_c - 25.) * self.dVoc_dT
        self.sat = self.voc >= self.v_sat

    def soc_est_ekf(self, x, dt, u, t_=None, z_=None, tau=None):
        """ innovation function for simple EKF per the Mathworks' paper. See Huria, Ceraolo, Gazzarri, & Jackey,
        2013 Simplified Extended Kalman Filter Observer for SOC Estimation of Commercial Power-Oriented LFP Lithium
        Battery Cells """
        # This works for 1x1 EKF
        out = np.empty_like(x)
        if tau is None:
            exp_t_tau = math.exp(-dt/self.tau_sd)
        else:
            exp_t_tau = math.exp(-dt/tau)
        out[0] = exp_t_tau*x[0] + (1. - exp_t_tau)*u[0]
        return out

    def look(self, temp_c):
        b = self.lut_b.lookup(T_degC=temp_c)
        a = self.lut_a.lookup(T_degC=temp_c)
        c = self.lut_c.lookup(T_degC=temp_c)
        return b, a, c

    def propagate_state_space_ekf(self, u, dt=None):
        # Time propagation from state-space using backward Euler
        # Inputs: [ib, vb].T
        # Outputs: voc
        self.u = u
        self.ib = self.u[0]
        self.vb = self.u[1]
        if self.x is None:  # Initialize
            self.x = np.array([self.ib * self.r_ct, self.ib * self.r_dif]).T
        if dt is not None:
            self.dt = dt
        self.x_past = self.x
        self.x_dot = self.A @ self.x + self.B @ self.u
        self.x += self.x_dot * self.dt
        self.y = self.C @ self.x_past + self.D @ self.u  # uses past (backward Euler)
        self.voc = self.y[0]
        self.ioc = self.ib


if __name__ == '__main__':
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt
    import book_format

    book_format.set_style()


    def main():
        # coefficient definition
        dt = 0.1
        time_end = 700
        # time_end = 1
        battery_model = Battery()
        t = np.arange(0, time_end + dt, dt)
        ib = []
        v_oc_s = []
        vbs = []
        vcs = []
        vds = []
        i_oc_s = []
        i_ct_s = []
        i_r_ct_s = []
        i_c_dif_s = []
        i_r_dif_s = []
        v_bc_dot_s = []
        v_cd_dot_s = []
        soc_s = []
        soc_norm_s = []
        pow_s = []
        for i in range(len(t)):
            if t[i] < 10:
                current_in = 0
            elif t[i] < 400:
                current_in = 30
            else:
                current_in = 0

            # Models
            battery_model.calc_voc(temp_c=25, soc_init=0.95)
            u = np.array([current_in, battery_model.voc]).T
            battery_model.calc_dynamics(u, dt=dt)

            # Plot stuff
            ib.append(battery_model.ib)
            v_oc_s.append(battery_model.voc)
            vbs.append(battery_model.vb)
            vcs.append(battery_model.vc())
            vds.append(battery_model.vd())
            i_oc_s.append(battery_model.ioc)
            i_ct_s.append(battery_model.i_c_ct())
            i_c_dif_s.append(battery_model.i_c_dif())
            i_r_ct_s.append(battery_model.i_r_ct())
            i_r_dif_s.append(battery_model.i_r_dif())
            v_bc_dot_s.append(battery_model.vbc_dot())
            v_cd_dot_s.append(battery_model.vcd_dot())
            soc_norm_s.append(battery_model.soc_norm)
            soc_s.append(battery_model.soc)
            pow_s.append(battery_model.pow_in)

        # Plots
        plt.figure()
        plt.subplot(321)
        plt.title('GP_battery.py')
        plt.plot(t, ib, color='green', label='I')
        plt.plot(t, i_r_ct_s, color='red', label='I_Rct')
        plt.plot(t, i_c_dif_s, color='cyan', label='I_C_dif')
        plt.plot(t, i_r_dif_s, color='orange', linestyle='--', label='I_R_dif')
        plt.plot(t, i_oc_s, color='orange', label='Ioc')
        plt.legend(loc=1)
        plt.subplot(323)
        plt.plot(t, vbs, color='green', label='Vb')
        plt.plot(t, vcs, color='blue', label='Vc_o')
        plt.plot(t, vds, color='red', label='Vd')
        plt.plot(t, v_oc_s, color='blue', label='Voc')
        plt.legend(loc=1)
        plt.subplot(325)
        plt.plot(t, v_bc_dot_s, color='green', label='Vbc_dot')
        plt.plot(t, v_cd_dot_s, color='blue', label='Vcd_dot')
        plt.legend(loc=1)
        plt.subplot(322)
        plt.plot(t, soc_s, color='green', label='SOC')
        plt.plot(t, soc_norm_s, color='red', label='SOC_norm')
        plt.legend(loc=1)
        plt.subplot(324)
        plt.plot(t, pow_s, color='orange', label='Pow_in')
        plt.legend(loc=1)
        plt.subplot(326)
        plt.plot(soc_norm_s, v_oc_s, color='black', label='voc vs soc_norm')
        plt.legend(loc=1)
        plt.show()


    main()
