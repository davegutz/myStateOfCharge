# GP_batteryEKF - general purpose battery class for EKF use
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
from numpy.random import randn
from pyDAGx.lookup_table import LookupTable
# from filterpy.kalman import ExtendedKalmanFilter
from pyDAGx.EKFx import ExtendedKalmanFilter


class Battery:
    # Battery model:  Randle's dynamics, SOC-VOC model

    def __init__(self, n_cells=4, r0=0.003, tau_ct=3.7, rct=0.0016, tau_dif=83, r_dif=0.0077, dt=0.1, b=0., a=0.,
                 c=0., n=0.4, m=0.478, d=0.707, t_t=None, t_b=None, t_a=None, t_c=None, nom_bat_cap=100.,
                 true_bat_cap=102., nom_sys_volt=13., dv=0, sr=1, bat_v_sat=3.4625, dvoc_dt=0.001875, sat_gain=10.,
                 tau_m=0.159):
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
        self.tau_m = tau_m  # Measurement time constant
        self.A, self.B, self.C, self.D = self.construct_state_space()
        self.u = np.array([0., 0]).T    # For dynamics
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
        self.s_sat = 0.  # Scalar feedback to mimic bms current cutback
        self.sat_gain = sat_gain  # Multiplier on saturation anti-windup
        self.voc = nom_sys_volt  # Battery charge voltage, V
        self.sat = True  # Battery is saturated, T/F
        self.ib = 0  # Current into battery post, A
        self.vb = nom_sys_volt  # Battery voltage at post, V
        self.ioc = 0  # Battery charge current, A
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
        self.v_batt = self.nom_sys_volt
        self.i_batt = 0.
        self.soc_norm_lim = 0.
        self.log_soc_norm = 0.
        self.exp_n_soc_norm = 0.
        self.pow_log_soc_norm = 0.

    def calc_voc(self, temp_c=25., soc_init=0.5):
        """SOC-OCV curve fit method per Zhang, et al
        Inputs:
            soc_norm        Normalized soc (0-1), fraction
        Outputs:
            voc             Charge voltage, V
            dV_dSoc_norm    SOC-VOC slope, V/fraction
            v_sat           Charge voltage at saturation, V
            sat             Battery is saturated, T/F
        """

        # Initialize
        if self.pow_in is None:
            self.soc = soc_init
            # self.soc_norm = 1. - (1. - self.soc) * self.cu / self.cs
            self.soc_norm = self.soc * self.cu / self.cs

        # Zhang coefficients
        self.b, self.a, self.c = self.look(temp_c)

        # Perform computationally expensive steps one time
        self.soc_norm_lim = max(min(self.soc_norm, self.eps_max), self.eps_min)
        self.log_soc_norm = math.log(self.soc_norm_lim)
        self.exp_n_soc_norm = math.exp(self.n * (self.soc_norm_lim - 1))
        self.pow_log_soc_norm = math.pow(-self.log_soc_norm, self.m)

        # SOC-OCV model
        self.dV_dSoc_norm = float(self.n_cells) * (self.b * self.m / self.soc_norm_lim * self.pow_log_soc_norm /
                                                   self.log_soc_norm + self.c + self.d * self.n * self.exp_n_soc_norm)
        self.dV_dSoc = self.dV_dSoc_norm * self.cu / self.cs
        # slightly beyond
        self.voc = float(self.n_cells) * (self.a + self.b * self.pow_log_soc_norm +
                                          self.c * self.soc_norm_lim + self.d * self.exp_n_soc_norm) + (
                               self.soc_norm - self.soc_norm_lim) * self.dV_dSoc_norm
        self.voc += self.dv  # Experimentally varied
        # self.d2V_dSocs2 = float(self.n_cells) * ( self.b*self.m/self.soc/self.soc*pow_log_soc_norm/
        # log_soc_norm*((self.m-1.)/log_soc_norm - 1.) + self.d*self.n*self.n*exp_n_soc_norm )
        self.v_sat = self.nom_v_sat + (temp_c - 25.) * self.dVoc_dT
        if self.ioc > 0:
            self.s_sat = max(self.voc - self.v_sat, 0.0) / self.nom_sys_volt * self.nom_bat_cap * self.sat_gain
        else:
            self.s_sat = 0.

        self.sat = self.voc >= self.v_sat
        self.u[1] = self.voc
        return self.voc

    def calc_dynamics(self, u, dt=None):
        """Executive model dynamics
        State-space calculation
        Inputs:
            ib  Battery terminal current, A
            voc Charge voltage, V
        Outputs:
            vb  Battery terminal voltage, V
            ioc Charge current, A
            v_batt  Sensed battery terminal voltage, v
            i_batt  Sensed battery terminal current, A
            time_charge Calculated time to full charge, hr
        """
        # Model dynamics executive
        if dt is not None:
            self.dt = dt
        ib = u[0]
        if self.sat and ib > 0:
            ib = ib - self.s_sat
        voc = u[1]
        self.u = np.array([ib, voc]).T

        # State-space calculation
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

    def construct_state_space(self):
        """ State-space representation of dynamics
        Inputs:
            ib      Current at V_bat = Vb = current at shunt, A
            voc     Internal open circuit voltage, V
        Outputs:
            vb      Voltage at positive, V
            ioc     Current into storage, A
            v_batt  Sensed battery terminal voltage, V
            i_batt  Sensed battery terminal current, A
        States:
            vbc     RC vb-vc, V
            vcd     RC vc-vd, V
            v_batt  Sensed circuit lag, V
            i_batt  Sensed circuit lag, A
        Other:
            vc      Voltage downstream of charge transfer model, ct-->c
            vd      Voltage downstream of diffusion process model, dif-->d
        """
        a = np.array([[-1/self.tau_ct,  0,                  0,              0],
                      [0,               -1/self.tau_dif,    0,              0],
                      [1/self.tau_m,    1/self.tau_m,       -1/self.tau_m,  0],
                      [0,               0,                  0,              -1/self.tau_m]])
        b = np.array([[1 / self.c_ct,       0],
                      [1 / self.c_dif,      0],
                      [self.r_0/self.tau_m, 1/self.tau_m],
                      [1/self.tau_m,        0]])
        c = np.array([[1,   1,  0,  0],
                      [0,   0,  0,  0],
                      [0,   0,  1,  0],
                      [0,   0,  0,  1]])
        d = np.array([[self.r_0,    1],
                      [1,           0],
                      [0,           0],
                      [0,           0]])
        return a, b, c, d

    def coulomb_counter(self):
        """Coulomb counter
        Internal resistance of battery is a loss
        Inputs:
            ioc     Charge current, A
            voc     Charge voltage, V
        Outputs:
            soc     State of charge, fraction (0-1.5)
            soc_norm    Normalized state of charge, fraction (0-1)
        """
        self.pow_in = self.ioc * self.voc * self.sr
        self.soc = max(min(self.soc + self.pow_in / self.nom_sys_volt * self.dt / 3600. / self.nom_bat_cap, 1.5), 0.)
        # if self.sat:
        #     self.soc = self.eps_max
        # self.soc_norm = 1. - (1. - self.soc) * self.cu / self.cs
        self.soc_norm = self.soc * self.cu / self.cs

    def look(self, temp_c):
        # Table lookups of Zhang coefficients
        b = self.lut_b.lookup(T_degC=temp_c)
        a = self.lut_a.lookup(T_degC=temp_c)
        c = self.lut_c.lookup(T_degC=temp_c)
        return b, a, c

    def propagate_state_space(self, u, dt=None):
        """Time propagation from state-space using backward Euler
        Inputs:
            ib      Measured battery terminal current, A
            voc     Calculated charge voltage, V
        Outputs:
            vb      Calculated battery terminal voltage, V
            ioc     Calculated charge current, A
            v_batt  Sensed circuit lag, V
            i_batt  Sensed battery terminal current, A
        """
        self.u = u
        self.ib = self.u[0]
        self.voc = self.u[1]
        if self.x is None:  # Initialize
            self.x = np.array([self.ib*self.r_ct, self.ib*self.r_dif, self.voc, self.ib]).T
        if dt is not None:
            self.dt = dt
        self.x_past = self.x
        self.x_dot = self.A @ self.x + self.B @ self.u
        self.x += self.x_dot * self.dt
        self.y = self.C @ self.x_past + self.D @ self.u  # uses past (backward Euler)
        self.vb = self.y[0]
        self.ioc = self.y[1]  # note, circuit construction constrains ioc=ib
        self.v_batt = self.y[2]
        self.i_batt = self.y[3]

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
        self.u = np.array([0., 0]).T    # For dynamics
        self.dt = dt
        self.x = None
        self.x_past = None
        self.x_dot = self.x
        self.y = np.array([0., 0]).T

        # SOC-VOC Battery coefficients
        self.temp_c = None
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
        self.exp_t_tau = math.exp(-dt / self.tau_sd)

        # Other things
        self.soc = 0.  # State of charge, %
        self.soc_norm = 0.  # State of charge, normalized, %
        self.soc_norm_filtered = 0.  # EKF normalized state of charge, %
        self.dV_dSoc = 0.  # Slope of soc-voc curve, V/%
        self.dV_dSoc_norm = 0.  # Slope of soc-voc curve, normalized, V/%
        self.nom_bat_cap = nom_bat_cap
        """ Nominal battery label capacity, e.g. 100, Ah. Accounts for internal losses.
            This is what gets delivered, e.g. W_shunt/NOM_SYS_VOLT.  Also varies 0.2-0.4C
            currents or 20-40 A for a 100 Ah battery"""
        self.nom_sys_volt = nom_sys_volt  # Nominal battery label voltage, e.g. 12, V
        self.v_sat = nom_sys_volt + 1.7  # Saturation voltage, V
        # self.voc = nom_sys_volt  # Battery charge voltage, V
        self.voc_dyn = nom_sys_volt  # Battery charge voltage calculated from dynamics, V
        self.voc_filtered = nom_sys_volt  # Battery charge voltage EKF, V
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
        self.soc_norm_lim = 0.
        self.log_soc_norm = 0.
        self.exp_n_soc_norm = 0.
        self.pow_log_soc_norm = 0.

    def calc_dynamics_ekf(self, u, dt=None):
        """Executive model dynamics for ekf
        State-space calculation
        Inputs:
            ib      Measured battery terminal current, A
            vb      Measured battery terminal voltage, V
        Outputs:
            ioc     Charge current, A
            voc_dyn Charge voltage dynamic calculation, V
        """
        if dt is not None:
            self.dt = dt
            # voc   Charge voltage, V
        self.u = u
        self.propagate_state_space_ekf(self.u, dt=dt)

    def calc_soc_voc_coeff(self, soc_k):
        """
        Calculate the Zhang coefficients for SOC-VOC model
        """
        # Zhang coefficients
        self.b, self.a, self.c = self.look(self.temp_c)

        # Perform computationally expensive steps one time
        self.soc_norm_filtered = soc_k
        self.soc_norm_lim = max(min(self.soc_norm_filtered, self.eps_max), self.eps_min)
        self.log_soc_norm = math.log(self.soc_norm_lim)
        self.exp_n_soc_norm = math.exp(self.n * (self.soc_norm_lim - 1))
        self.pow_log_soc_norm = math.pow(-self.log_soc_norm, self.m)

    def h_jacobian(self, soc_k):
        """SOC-OCV slope calculation per Zhang, et al
            Inputs:
                soc_k           Normalized soc from ekf (0-1), fraction
            Outputs:
                dV_dSoc_norm    SOC-VOC slope, V/fraction
        """
        # Calculate the coefficients
        self.calc_soc_voc_coeff(soc_k)

        # SOC-OCV model
        self.dV_dSoc_norm = float(self.n_cells) * (self.b * self.m / self.soc_norm_lim * self.pow_log_soc_norm /
                                                   self.log_soc_norm + self.c + self.d * self.n * self.exp_n_soc_norm)
        self.dV_dSoc = self.dV_dSoc_norm * self.cu / self.cs
        return self.dV_dSoc_norm

    def hx_calc_voc(self, soc_k):
        """SOC-OCV curve fit method per Zhang, et al
        The non-linear 'y' function for EKF
        Inputs:
            soc_k           Normalized soc from ekf (0-1), fraction
            dV_dSoc_norm    SOC-VOC slope, V/fraction
        Outputs:
            voc             Charge voltage, V
        """
        # SOC-OCV model
        self.voc_filtered = float(self.n_cells) * (self.a + self.b * self.pow_log_soc_norm +
                                                   self.c * self.soc_norm_lim + self.d * self.exp_n_soc_norm)
        self.voc_filtered += self.dv  # Experimentally varied
        # slightly beyond
        self.voc_filtered += (self.soc_norm_filtered - self.soc_norm_lim) * self.dV_dSoc_norm
        return self.voc_filtered

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

    def coulomb_counter_ekf(self):
        """Coulomb counter
        Internal resistance of battery is a loss
        Inputs:
            ioc     Charge current, A
            voc     Charge voltage, V
            i_r_dif Current in diffusion process, A
            i_r_ct  Current in charge transfer process, A
            sr      Experimental scalar
        Outputs:
            soc     State of charge, fraction (0-1.5)
            soc_norm    Normalized state of charge, fraction (0-1)
            v_sat   Charge voltage at saturation, V
            sat     Battery is saturated, T/F
        """
        self.pow_in = self.ib * (self.vb - (self.ioc * self.r_0 + self.i_r_dif() * self.r_dif +
                                            self.i_r_ct() * self.r_ct) * self.sr)
        self.soc = max(min(self.soc + self.pow_in / self.nom_sys_volt * self.dt / 3600. / self.nom_bat_cap, 1.5), 0.)
        # if self.sat:
        #     self.soc = self.eps_max
        # self.soc_norm = 1. - (1. - self.soc) * self.cu / self.cs
        self.soc_norm = self.soc * self.cu / self.cs
        self.v_sat = self.nom_v_sat + (self.temp_c - 25.) * self.dVoc_dT
        self.sat = self.voc_dyn >= self.v_sat

    def h_ekf(self, x):
        """ EKF feedback function
        Inputs:
            x   EKF state
        Outputs:
            y   EKF output from non-linear function
        """
        return [self.hx_calc_voc(x)]

    def look(self, temp_c):
        b = self.lut_b.lookup(T_degC=temp_c)
        a = self.lut_a.lookup(T_degC=temp_c)
        c = self.lut_c.lookup(T_degC=temp_c)
        return b, a, c

    def propagate_state_space_ekf(self, u, dt=None):
        """Time propagation from state-space using backward Euler
        Inputs:
            ib      Measured current into positive battery terminal, A
            vb      Measured battery terminal voltage, V
        Outputs:
            ioc     Calculated charge current, A
            voc_dyn Calculated charge voltage dynamic, V
        """
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
        self.ioc = self.ib
        self.voc_dyn = self.y

    def fx_soc_est(self, x, dt, u, tau=None):
        """ Innovation function for simple EKF per the Mathworks' paper. See Huria, Ceraolo, Gazzarri, & Jackey,
        2013, 'Simplified Extended Kalman Filter Observer for SOC Estimation of Commercial Power-Oriented LFP Lithium
        Battery Cells'
        This works for 1x1 EKF
        Input:
          ib  Measured battery terminal current, A
        Output:
          voc (soc)   `Estimated soc, V equivalent
        """
        if tau is not None:
            self.exp_t_tau = math.exp(-dt/tau)
        out = self.exp_t_tau*x + (1. - self.exp_t_tau)*u
        return out

    def assign_soc(self, soc, voc):
        self.soc = soc
        self.voc_filtered = voc
        self.v_sat = self.nom_v_sat + (self.temp_c - 25.) * self.dVoc_dT
        self.sat = self.voc_filtered >= self.v_sat

    def assign_soc_norm(self, soc_norm, voc):
        self.soc_norm = soc_norm
        self.soc = soc_norm * self.cs / self.cu
        self.voc_filtered = voc
        self.v_sat = self.nom_v_sat + (self.temp_c - 25.) * self.dVoc_dT
        self.sat = self.voc_filtered >= self.v_sat

    def assign_temp_c(self, temp_c):
        self.temp_c = temp_c

    def i_c_ct(self):
        return self.vbc_dot() * self.c_ct

    def i_c_dif(self):
        return self.vcd_dot() * self.c_dif

    def i_r_ct(self):
        return self.vbc() / self.r_ct

    def i_r_dif(self):
        return self.vcd() / self.r_dif

    def vbc(self):
        return self.x[0]

    def vcd(self):
        return self.x[1]

    def vbc_dot(self):
        return self.x_dot[0]

    def vcd_dot(self):
        return self.x_dot[1]


if __name__ == '__main__':
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt
    import book_format

    book_format.set_style()


    def main():
        # Setup to run the transients
        dt = 0.1
        dt_ekf = 0.1
        # time_end = 1
        # time_end = 13.3
        time_end = 700
        # time_end = 1400
        temp_c = 25.
        soc_init = 0.975

        # Definition
        battery_model = Battery()
        battery_ekf = BatteryEKF()
        v_std = 0.01  # batt voltage meas uncertainty (0.01)
        i_std = 0.1  # shunt current meas uncertainty (0.1)

        # Setup the EKF
        r_std = 0.1  # Kalman sensor uncertainty (0.1) belief in meas
        q_std = 0.001  # Process uncertainty (0.001) belief in state
        kf = ExtendedKalmanFilter(dim_x=1, dim_z=1)
        kf.R = r_std**2
        kf.Q = q_std**2
        kf.P *= 100

        # Executive tasks
        t = np.arange(0, time_end + dt, dt)
        current_in_s = []
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
        soc_norm_ekf_s = []
        voc_dyn_s = []
        soc_norm_filtered_s = []
        voc_filtered_s = []
        prior_soc_s = []
        x_s = []
        z_s = []
        k_s = []
        v_batt_s = []
        i_batt_s = []

        for i in range(len(t)):
            if t[i] < 50:
                current_in = 0.
            elif t[i] < 450:
                current_in = 40.
            elif t[i] < 550:
                current_in = 0.
            elif t[i] < 950:
                current_in = -40.
            else:
                current_in = 0.
            init_ekf = (i <= 100)

            # Models
            battery_model.calc_voc(temp_c=temp_c, soc_init=soc_init)
            u = np.array([current_in, battery_model.voc]).T
            battery_model.calc_dynamics(u, dt=dt)

            # EKF
            if init_ekf:
                battery_ekf.assign_temp_c(temp_c)
                battery_ekf.assign_soc_norm(float(battery_model.soc_norm), battery_model.voc)
                kf.x = np.array([battery_model.soc_norm])
            # Setup
            u_ekf = np.array([battery_model.i_batt+randn()*i_std, battery_model.v_batt+randn()*v_std]).T
            battery_ekf.calc_dynamics_ekf(u_ekf, dt=dt_ekf)
            battery_ekf.coulomb_counter_ekf()
            # Call Kalman Filter
            kf.predict(u=battery_ekf.ib)
            kf.update(battery_ekf.voc_dyn, battery_ekf.h_jacobian, battery_ekf.hx_calc_voc)

            # Plot stuff
            current_in_s.append(current_in)
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
            soc_norm_ekf_s.append(battery_ekf.soc_norm)
            voc_dyn_s.append(battery_ekf.voc_dyn)
            soc_norm_filtered_s.append(battery_ekf.soc_norm_filtered)
            voc_filtered_s.append(battery_ekf.voc_filtered)
            prior_soc_s.append(kf.x_prior[0])
            x_s.append(kf.x)
            z_s.append(kf.z)
            k_s.append(kf.K)
            v_batt_s.append(battery_model.v_batt)
            i_batt_s.append(battery_model.i_batt)

        # Plots
        if False:
            plt.figure()
            plt.subplot(321)
            plt.title('GP_battery_EKF - Model.py')
            plt.plot(t, current_in_s, color='black', label='current demand, A')
            plt.plot(t, ib, color='green', label='ib')
            plt.plot(t, i_r_ct_s, color='red', label='I_Rct')
            plt.plot(t, i_c_dif_s, color='cyan', label='I_C_dif')
            plt.plot(t, i_r_dif_s, color='orange', linestyle='--', label='I_R_dif')
            plt.plot(t, i_oc_s, color='black', linestyle='--', label='Ioc')
            plt.legend(loc=1)
            plt.subplot(323)
            plt.plot(t, vbs, color='green', label='Vb')
            plt.plot(t, vcs, color='blue', label='Vc')
            plt.plot(t, vds, color='red', label='Vd')
            plt.plot(t, v_oc_s, color='orange', label='Voc')
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

        if True:
            plt.figure()
            plt.subplot(321)
            plt.title('GP_battery_EKF - Filter')
            plt.plot(t, ib, color='black', label='ib')
            plt.plot(t, i_batt_s, color='magenta', linestyle='dotted', label='i_batt')
            plt.legend(loc=3)
            plt.subplot(322)
            plt.plot(t, soc_norm_s, color='red', label='SOC_norm')
            plt.plot(t, soc_norm_ekf_s, color='black', linestyle='dotted', label='SOC_norm_ekf')
            # plt.ylim(0.92, 1.0)
            plt.legend(loc=4)
            plt.subplot(323)
            plt.plot(t, v_oc_s, color='blue', label='actual voc')
            plt.plot(t, voc_dyn_s, color='red', linestyle='dotted', label='voc_dyn meas')
            plt.plot(t, voc_filtered_s, color='green', label='voc_filtered state')
            plt.plot(t, vbs, color='black', label='vb')
            plt.plot(t, v_batt_s, color='magenta', linestyle='dotted', label='v_batt')
            # plt.ylim(13, 15)
            plt.legend(loc=4)
            plt.subplot(324)
            plt.plot(t, soc_norm_s, color='black', linestyle='dotted', label='SOC_norm')
            plt.plot(t, prior_soc_s, color='red', linestyle='dotted', label='post soc_norm_filtered')
            plt.plot(t, x_s, color='green', label='x soc_norm_filtered')
            # plt.ylim(0.92, 1.0)
            plt.legend(loc=4)
            plt.subplot(325)
            plt.plot(t, k_s, color='red', linestyle='dotted', label='K (belief state / belief meas)')
            plt.legend(loc=4)
            plt.show()


    main()
