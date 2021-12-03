# BatteryEKF - general purpose battery class for embedded EKF
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

"""Define a SoC-VOV model as well as Kalman filtering support for simplified Mathworks' tracker. See Huria, Ceraolo,
Gazzarri, & Jackey, 2013 Simplified Extended Kalman Filter Observer for SOC Estimation of Commercial Power-Oriented LFP
Lithium Battery Cells """

import numpy as np
import math
from pyDAGx.lookup_table import LookupTable


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
        self.Fx = self.exp_t_tau
        self.Bu = (1. - self.exp_t_tau)*self.r_sd
        self.Q = 0.
        self.R = 0.
        self.P = 0.
        self.H = 0.
        self.S = 0.
        self.K = 0.
        self.hx = 0.
        self.u_kf = 0.
        self.x_kf = 0.
        self.y_kf = 0.
        self.z_ekf = 0.
        self.x_prior = self.x_kf
        self.P_prior = self.P
        self.x_post = self.x_kf
        self.P_post = self.P

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
        self.voc = nom_sys_volt  # Battery charge voltage, V
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
        """Executive model dynamics for ekf State-space calculation
        Time propagation from state-space using backward Euler
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

    def calc_voc_solve(self, soc, dv_dsoc):
        """SOC-OCV curve fit method per Zhang, et al
        The non-linear 'y' function for EKF
        Inputs:
            soc_k           Normalized soc from ekf (0-1), fraction
            dV_dSoc_norm    SOC-VOC slope, V/fraction
        Outputs:
            voc             Charge voltage, V
        """
        # SOC-OCV model
        soc_norm_lim = max(min(soc, self.eps_max), self.eps_min)
        log_soc_norm = math.log(soc_norm_lim)
        exp_n_soc_norm = math.exp(self.n * (soc_norm_lim - 1))
        pow_log_soc_norm = math.pow(-log_soc_norm, self.m)
        voc_filtered = float(self.n_cells) * (self.a + self.b * pow_log_soc_norm +
                                                   self.c * soc_norm_lim + self.d * exp_n_soc_norm)
        voc_filtered += self.dv  # Experimentally varied
        # slightly beyond
        voc_filtered += (soc - soc_norm_lim) * dv_dsoc
        return voc_filtered

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
        self.v_sat = self.nom_v_sat + (self.temp_c - 25.) * self.dVoc_dT
        self.sat = self.voc_dyn >= self.v_sat
        if self.sat:
            self.soc = self.eps_max
        # self.soc_norm = 1. - (1. - self.soc) * self.cu / self.cs
        self.soc_norm = self.soc * self.cu / self.cs

    def kf_predict_1x1(self, u=None):
        """1x1 Extended Kalman Filter predict
        Inputs:
            u   1x1 input, =ib, A
            Bu  1x1 control transition, Ohms
            Fx  1x1 state transition, V/V
        Outputs:
            x   1x1 Kalman state variable = Vsoc (0-1 fraction)
            P   1x1 Kalman probability
        """
        self.u_kf = u
        self.x_kf = self.Fx*self.x_kf + self.Bu*self.u_kf
        self.P = self.Fx * self.P * self.Fx + self.Q
        self.x_prior = self.x_kf
        self.P_prior = self.P

    def kf_update_1x1(self, z, h_jacobian=None, hx_calc=None):
        """ 1x1 Extended Kalman Filter update
            Inputs:
                z   1x1 input, =voc, dynamic predicted by other model, V
                R   1x1 Kalman state uncertainty
                Q   1x1 Kalman process uncertainty
                H   1x1 Jacobian sensitivity dV/dSOC
            Outputs:
                x   1x1 Kalman state variable = Vsoc (0-1 fraction)
                y   1x1 Residual z-hx, V
                P   1x1 Kalman uncertainty covariance
                K   1x1 Kalman gain
                S   1x1 system uncertainty
                SI  1x1 system uncertainty inverse
        """
        if h_jacobian is None:
            h_jacobian = self.h_jacobian
        if hx_calc is None:
            hx_calc = self.hx_calc_voc
        self.z_ekf = z
        self.H = h_jacobian(self.x_kf)
        PHT = self.P*self.H
        self.S = self.H*PHT + self.R
        self.K = PHT/self.S
        self.hx = hx_calc(self.x_kf)
        self.y_kf = self.z_ekf - self.hx
        self.x_kf = self.x_kf + self.K*self.y_kf
        I_KH = 1 - self.K*self.H
        self.P = I_KH*self.P
        self.x_post = self.x_kf
        self.P_post = self.P

    def look(self, temp_c):
        # Lookup Zhang coefficients
        b = self.lut_b.lookup(T_degC=temp_c)
        a = self.lut_a.lookup(T_degC=temp_c)
        c = self.lut_c.lookup(T_degC=temp_c)
        return b, a, c

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
