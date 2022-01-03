# Battery - general purpose battery class for modeling
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

"""Define a general purpose battery model including Randle's model and SoC-VOV model."""

import numpy as np
import math
from pyDAGx.lookup_table import LookupTable
from EKF_1x1 import EKF_1x1
from Coulombs import Coulombs

# Battery constants
RATED_BATT_CAP = 100.
"""Nominal battery bank capacity, Ah(100).Accounts for internal losses.This is
                        what gets delivered, e.g.Wshunt / NOM_SYS_VOLT.  Also varies 0.2 - 0.4 C currents
                        or 20 - 40 A for a 100 Ah battery"""
RATED_TEMP = 25.  # Temperature at RATED_BATT_CAP, deg C
BATT_DVOC_DT = 0.001875
""" Change of VOC with operating temperature in range 0 - 50 C (0.0075) V/deg C
                            >3.425 V is reliable approximation for SOC>99.7 observed in my prototype around 15-35 C"""
BATT_V_SAT = 3.4625  # Normal battery cell saturation for SOC=99.7, V (3.4625 = 13.85v)
NOM_SYS_VOLT = 12.  # Nominal system output, V, at which the reported amps are used (12)
mxeps_bb = 1-1e-6  # Numerical maximum of coefficient model with scaled soc
mneps_bb = 1e-6  # Numerical minimum of coefficient model without scaled soc
DQDT = 0.01  # Change of charge with temperature, fraction/deg C.  From literature.  0.01 is commonly used
CAP_DROOP_C = 20.  # Temperature below which a floor on q arises, C (20)


class Battery(Coulombs, EKF_1x1):
    # Battery model:  Randle's dynamics, SOC-VOC model

    def __init__(self, t_t=None, t_b=None, t_a=None, t_c=None, m=0.478, n=0.4, d=0.707, nz=None,
                 num_cells=4, bat_v_sat=3.4625, dvoc_dt_=BATT_DVOC_DT, q_cap_rated=RATED_BATT_CAP*3600,
                 t_rated=RATED_TEMP, t_rlim=0.017):  # See Battery.h
        """ Default values from Taborelli & Onori, 2013, State of Charge Estimation Using Extended Kalman Filters for
        Battery Management System.   Battery equations from LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang
        etal.pdf.'  SOC-OCV curve fit './Battery State/BattleBorn Rev1.xls:Model Fit' using solver with min slope
        constraint >=0.02 V/soc.  m and n using Zhang values.   Had to scale soc because  actual capacity > NOM_BAT_CAP
        so equations error when soc<=0 to match data.    See Battery.h
        """
        # Parents
        Coulombs.__init__(self, q_cap_rated, t_rated, t_rlim)

        # Defaults
        if t_t is None:
            t_t = [0., 25., 50.]
        if t_b is None:
            t_b = [-0.836, -0.836, -0.836]
        if t_a is None:
            t_a = [3.999, 4.046, 4.093]
        if t_c is None:
            t_c = [-1.181, -1.181, -1.181]
        self.num_cells = num_cells
        self.b = 0
        self.a = 0
        self.c = 0
        self.d = d
        self.n = n
        self.m = m
        self.nz = None
        self.lut_b = LookupTable()
        self.lut_a = LookupTable()
        self.lut_c = LookupTable()
        self.lut_b.addAxis('T_degC', t_t)
        self.lut_a.addAxis('T_degC', t_t)
        self.lut_c.addAxis('T_degC', t_t)
        self.lut_b.setValueTable(t_b)
        self.lut_a.setValueTable(t_a)
        self.lut_c.setValueTable(t_c)
        self.q = 0  # Charge, C
        self.voc = NOM_SYS_VOLT  # Static model open circuit voltage, V
        self.vdyn = 0.  # Model current induced back emf, V
        self.vb = NOM_SYS_VOLT  # Battery voltage at post, V
        self.ib = 0  # Current into battery post, A
        self.num_cells = num_cells
        self.dv_dsoc = 0.  # Slope of soc-voc curve, V/%
        self.tcharge = 0.  # Charging time to 100%, hr
        self.sr = 1  # Resistance scalar
        self.nom_vsat = bat_v_sat * self.num_cells  # Normal battery cell saturation for SOC=99.7, V (3.4625 = 13.85v)
        self.vsat = NOM_SYS_VOLT + 1.7  # Saturation voltage, V
        self.dv = 0  # Adjustment for voltage level, V (0)
        self.dVoc_dT = BATT_DVOC_DT * self.num_cells  # Change of VOC with operating temperature in
        # range 0 - 50 C, V/deg C
        self.dt = 0  # Update time, s
        self.r0 = 0.003  # Randles R0, ohms
        self.tau_ct = 0.2  # Randles charge transfer time constant, s (=1/Rct/Cct)
        self.rct = 0.0016  # Randles charge transfer resistance, ohms
        self.tau_dif = 83  # Randles diffusion time constant, s (=1/Rdif/Cdif)
        self.r_dif = 0.0077  # Randles diffusion resistance, ohms
        self.tau_sd = 1.8e7  # Time constant of ideal battery capacitor model, input current A, output volts=soc (0-1)
        self.r_sd = 70  # Trickle discharge of ideal battery capacitor model, ohms
        self.A, self.B, self.C, self.D = self.construct_state_space()
        self.temp_c = 25.
        self.tcharge_ekf = 0.  # Charging time to 100% from ekf, hr
        self.voc_dyn = 0.  # Charging voltage, V
        self.soc_ekf = 0.  # Filtered state of charge from ekf (0-1)
        self.SOC_ekf = 0.  # Filtered state of charge from ekf (0-100)
        self.q_ekf = 0  # Filtered charge calculated by ekf, C
        self.amp_hrs_remaining = 0  # Discharge amp*time left if drain to q=0, A-h
        self.amp_hrs_remaining_ekf = 0  # Discharge amp*time left if drain to q_ekf=0, A-h

    def calc_h_jacobian(self, soc_lim=0., b=0., c=0., log_soc=0., exp_n_soc=0., pow_log_soc=0.):
        dv_dsoc = float(self.num_cells) * (b * self.m / soc_lim * pow_log_soc / log_soc + c +
                                           self.d * self.n * exp_n_soc)
        return dv_dsoc

    def calc_soc_voc(self, soc_lim=0., b=0., a=0., c=0., log_soc=0., exp_n_soc=0., pow_log_soc=0.):
        """SOC-OCV curve fit method per Zhang, et al """
        dv_dsoc = self.calc_h_jacobian(soc_lim, b, c, log_soc, exp_n_soc, pow_log_soc)
        voc = self.num_cells * (a + b * pow_log_soc + c * soc_lim + self.d * exp_n_soc);
        # d2v_dsoc2_ = double(num_cells_) * (
        #       b * m_ / soc_ / soc_ * pow_log_soc / log_soc * ((m_ - 1.) / log_soc - 1.) + d_ * n_ * n_ * exp_n_soc);
        return voc, dv_dsoc

    def calc_soc_voc_coeff(self, soc=0., tc=25., n=0., m=0.):
        """SOC-OCV curve fit method per Zhang, et al """
        # Zhang coefficients
        b, a, c = self.look(tc)

        # Perform computationally expensive steps one time
        log_soc_norm = math.log(soc)
        exp_n_soc_norm = math.exp(n * (soc - 1))
        pow_log_soc_norm = math.pow(-log_soc_norm, m)
        return b, a, c, log_soc_norm, exp_n_soc_norm, pow_log_soc_norm

    def calculate(self):
        raise NotImplementedError

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
        Other:
            vc      Voltage downstream of charge transfer model, ct-->c
            vd      Voltage downstream of diffusion process model, dif-->d
        """
        c_ct = self.tau_ct / self.rct
        c_dif = self.tau_ct / self.rct
        a = np.array([[-1 / self.tau_ct, 0],
                      [0, -1 / self.tau_dif]])
        b = np.array([[1 / c_ct,   0],
                      [1 / c_dif,  0]])
        c = np.array([-1., -1])
        d = np.array([-self.r0, 1])
        return a, b, c, d

    def ekf_model_predict(self):
        """Process model"""
        self.Fx = math.exp(-self.dt / self.tau_sd)
        self.Bu = (1. - self.Fx)*self.r_sd

    def ekf_model_update(self):
        # Measurement function hx(x), x = soc ideal capacitor
        x_lim = max(min(self.x_kf, mxeps_bb), mneps_bb)
        b, a, c, log_soc, exp_n_soc, pow_log_soc = self.calc_soc_voc_coeff(x_lim, self.temp_c, self.n, self.m)
        self.hx, self.dv_dsoc = self.calc_soc_voc(x_lim, b, a, c, log_soc, exp_n_soc, pow_log_soc)
        # Jacobian of measurement function
        self.H = self.dv_dsoc
        return self.hx, self.H

    def look(self, temp_c):
        # Table lookups of Zhang coefficients
        b = self.lut_b.lookup(T_degC=temp_c)
        a = self.lut_a.lookup(T_degC=temp_c)
        c = self.lut_c.lookup(T_degC=temp_c)
        return b, a, c


class BatteryModel(Battery):
    """Extend basic monitoring class to run a model"""
    def __init__(self, t_t=None, t_b=None, t_a=None, t_c=None, m=0.478, n=0.4, d=0.707, nz=None,
        num_cells=4, bat_v_sat=3.4625, dvoc_dt=BATT_DVOC_DT, q_cap_rated=RATED_BATT_CAP*3600, t_rated=RATED_TEMP,
        t_rlim=0.017):
        Battery.__init__(t_t, t_b, t_a, t_c, m, n, d, nz, num_cells, bat_v_sat, dvoc_dt, q_cap_rated,
                         t_rated, t_rlim)

    def calculate(self, temp_c_=0., soc_=0., curr_in_=0., dt_=0., q_capacity_=0., q_cap_=0.):
        return#########################

    def count_coulombs(self, dt=0., reset=False, temp_c=25., charge_curr=0., t_last=0.):
        """Coulomb counter based on true=actual capacity
        Internal resistance of battery is a loss
        Inputs:
            dt              Integration step, s
            temp_c          Battery temperature, deg C
            charge_curr     Charge, A
            sat             Indicator that battery is saturated (VOC>threshold(temp)), T/F
            t_last          Past value of battery temperature used for rate limit memory, deg C
        Outputs:
            soc     State of charge, fraction (0-1.5)
        """
        d_delta_q = charge_curr * dt

        # Rate limit temperature
        temp_lim = max(min(temp_c, t_last + self.t_rlim*dt), t_last - self.t_rlim*dt)
        if reset:
            temp_lim = temp_c

        # Saturation.   Goal is to set q_capacity and hold it so remember last saturation status
        # detection
        if self.model_saturated:
            if reset:
                self.delta_q = 0.  # Model is truth.   Saturate it then restart it to reset charge
        self.resetting = False  # one pass flag.  Saturation debounce should reset next pass

        # Integration
        self.q_capacity = self.calculate_capacity(temp_lim)
        self.delta_q = max(min(self.delta_q + d_delta_q - DQDT*self.q_capacity*(temp_lim-self.t_last), 0.),
                           -self.q_capacity)
        self.q = self.q_capacity + self.delta_q

        # Normalize
        self.soc = self.q / self.q_capacity
        self.soc_min = max((CAP_DROOP_C - temp_lim)*DQDT, 0.)
        self.q_min = self.soc_min * self.q_capacity
        self.SOC = self.q / self.q_cap_rated_scaled * 100.

        # Save and return
        self.t_last = temp_lim
        return self.soc



class BatteryOld:
    # Battery model:  Randle's dynamics, SOC-VOC model

    def __init__(self, n_cells=4, r0=0.003, tau_ct=3.7, rct=0.0016, tau_dif=83, r_dif=0.0077, dt=0.1, b=0., a=0.,
                 c=0., n=0.4, m=0.478, d=0.707, t_t=None, t_b=None, t_a=None, t_c=None, nom_bat_cap=100.,
                 true_bat_cap=102., nom_sys_volt=13., dv=0, sr=1, bat_v_sat=3.4625, dvoc_dt=0.001875, sat_gain=10.,
                 tau_m=0.159, bat_cap_scalar=1.0, dqdt=0.01, temp_c=25.):
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
        self.u = np.array([0., 0]).T  # For dynamics
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
        self.true_bat_cap = true_bat_cap
        self.cs = true_bat_cap  # Data fit to this capacity to avoid math 0, Ah
        self.bat_cap_scalar = bat_cap_scalar  # Warp the battery char, e.g. life
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
        self.i_charge = self.ioc

        # New book-keep stuff (based on actual=true)
        self.temp_c = temp_c
        self.dqdt = dqdt  # Change of charge with temperature, soc/deg C
        self.delta_soc = 0.
        self.temp_c_init = self.temp_c
        self.charge_init = (self.temp_c_init - 25.) * self.dqdt + self.true_bat_cap
        self.soc_init = self.charge_init / self.true_bat_cap
        self.soc_avail = 1.

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

    def calc_dynamics(self, u, temp_c, dt=None, i_hyst=0.):
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

        # Crude hysteresis model
        if self.i_batt >= 0.:
            self.i_batt = max(self.i_batt - i_hyst, 0.0)
        else:
            self.i_batt = min(self.i_batt + i_hyst, 0.0)
        self.i_charge = self.i_batt

        # Coulomb counter
        self.coulomb_counter()
        self.coulomb_counter_avail(temp_c)

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
        a = np.array([[-1 / self.tau_ct, 0, 0, 0],
                      [0, -1 / self.tau_dif, 0, 0],
                      [1 / self.tau_m, 1 / self.tau_m, -1 / self.tau_m, 0],
                      [0, 0, 0, -1 / self.tau_m]])
        b = np.array([[1 / self.c_ct, 0],
                      [1 / self.c_dif, 0],
                      [self.r_0 / self.tau_m, 1 / self.tau_m],
                      [1 / self.tau_m, 0]])
        c = np.array([[1, 1, 0, 0],
                      [0, 0, 0, 0],
                      [0, 0, 1, 0],
                      [0, 0, 0, 1]])
        d = np.array([[self.r_0, 1],
                      [1, 0],
                      [0, 0],
                      [0, 0]])
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
        self.pow_in = self.i_charge * self.voc * self.sr
        self.soc = max(min(self.soc + self.pow_in / self.nom_sys_volt * self.dt / 3600. / self.nom_bat_cap, 1.5), 0.)
        # if self.sat:
        #     self.soc = self.eps_max
        # self.soc_norm = 1. - (1. - self.soc) * self.cu / self.cs
        self.soc_norm = self.soc * self.cu / self.cs

    def coulomb_counter_avail(self, temp_c):
        """Coulomb counter based on true
        Internal resistance of battery is a loss
        Inputs:
            ioc     Charge current, A
            voc     Charge voltage, V
        Outputs:
            soc_avail   State of charge, fraction (0-1.5)
            soc_norm    Normalized state of charge, fraction (0-1)
        """
        self.temp_c = temp_c
        self.pow_in = self.i_charge * self.voc * self.sr
        self.delta_soc = max(min(self.delta_soc + self.pow_in / self.nom_sys_volt * self.dt / 3600. / self.true_bat_cap,
                                 1.5), -1.5)
        if self.sat:
            self.delta_soc = 0.
            self.temp_c_init = self.temp_c
            self.charge_init = ((self.temp_c_init - 25.) * self.dqdt + 1.) * self.true_bat_cap
            self.soc_init = self.charge_init / self.true_bat_cap
        self.soc_avail = self.charge_init / self.true_bat_cap * (1. - self.dqdt * (self.temp_c - self.temp_c_init)) \
            + self.delta_soc

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
            self.x = np.array([self.ib * self.r_ct, self.ib * self.r_dif, self.voc, self.ib]).T
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
