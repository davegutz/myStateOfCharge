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
from StateSpace import StateSpace


class Retained:

    def __init__(self):
        self.cutback_gain_scalar = 1.


rp = Retained()

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
TCHARGE_DISPLAY_DEADBAND = 0.1  # Inside this +/- deadband, charge time is displayed '---', A
max_voc = 1.2*NOM_SYS_VOLT  # Prevent windup of battery model, V
batt_num_cells = int(NOM_SYS_VOLT/3)  # Number of standard 3 volt LiFePO4 cells
batt_vsat = float(batt_num_cells)*BATT_V_SAT  # Total bank saturation for 0.997=soc, V
batt_vmax = (14.3/4)*float(batt_num_cells)  # Observed max voltage of 14.3 V at 25C for 12V prototype bank, V


class Battery(Coulombs, EKF_1x1):
    # Battery model:  Randle's dynamics, SOC-VOC model

    def __init__(self, t_t=None, t_b=None, t_a=None, t_c=None, m=0.478, n=0.4, d=0.707,
                 num_cells=4, bat_v_sat=3.4625, q_cap_rated=RATED_BATT_CAP*3600,
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
        self.dvoc_dt = BATT_DVOC_DT * self.num_cells  # Change of VOC with operating temperature in
        # range 0 - 50 C, V/deg C
        self.dt = 0  # Update time, s
        self.r0 = 0.003  # Randles R0, ohms
        self.tau_ct = 0.2  # Randles charge transfer time constant, s (=1/Rct/Cct)
        self.rct = 0.0016  # Randles charge transfer resistance, ohms
        self.tau_dif = 83  # Randles diffusion time constant, s (=1/Rdif/Cdif)
        self.r_dif = 0.0077  # Randles diffusion resistance, ohms
        self.tau_sd = 1.8e7  # Time constant of ideal battery capacitor model, input current A, output volts=soc (0-1)
        self.r_sd = 70  # Trickle discharge of ideal battery capacitor model, ohms
        self.Randles = StateSpace()
        self.Randles.A, self.Randles.B, self.Randles.C, self.Randles.D = self.construct_state_space_monitor()
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
        voc = self.num_cells * (a + b * pow_log_soc + c * soc_lim + self.d * exp_n_soc)
        return voc, dv_dsoc

    def calc_soc_voc_coeff(self, soc=0., tc=25., n=0., m=0.):
        """SOC-OCV curve fit method per Zhang, et al """
        # Zhang coefficients
        b, a, c = self.look(tc)
        log_soc_norm = math.log(soc)
        exp_n_soc_norm = math.exp(n * (soc - 1))
        pow_log_soc_norm = math.pow(-log_soc_norm, m)
        return b, a, c, log_soc_norm, exp_n_soc_norm, pow_log_soc_norm

    def calculate(self):
        raise NotImplementedError

    def calculate_ekf(self, temp_c, vb, ib, dt):
        self.temp_c = temp_c
        self.vsat = calc_vsat(self.temp_c)

        # Dynamics
        self.vb = vb
        self.ib = ib
        u = [ib, vb]
        self.Randles.calc_x_dot(u)
        self.Randles.update(dt)
        self.voc_dyn = self.Randles.y[0]
        self.vdyn = self.vb - self.voc_dyn
        self.voc = self.voc_dyn

        # EKF 1x1
        self.predict_ekf(ib)
        self.update_ekf(self.voc_dyn, mneps_bb, mxeps_bb)
        self.soc_ekf = self.x_kf
        self.q_ekf = self.soc_ekf * self.q_capacity
        self.SOC_ekf = self.q_ekf / self.q_cap_rated_scaled * 100.

        # Charge time
        if self.ib > 0.1:
            self.tcharge_ekf = min(RATED_BATT_CAP/self.ib * (1. - self.soc_ekf), 24.)
        elif self.ib < -0.1:
            self.tcharge_ekf = max(RATED_BATT_CAP/self.ib * self.soc_ekf, -24.)
        elif self.ib >= 0.:
            self.tcharge_ekf = 24.*(1. - self.soc_ekf)
        else:
            self.tcharge_ekf = -24.*self.soc_ekf

        return self.soc_ekf

    def calculate_charge_time(self, q, q_capacity, charge_curr, soc):
        delta_q = q - q_capacity
        if charge_curr > TCHARGE_DISPLAY_DEADBAND:
            self.tcharge = min(-delta_q / charge_curr / 3600., 24.)
        elif charge_curr < -TCHARGE_DISPLAY_DEADBAND:
            self.tcharge = max(max(q_capacity + delta_q - self.q_min, 0.) / charge_curr / 3600., -24.)
        elif charge_curr >= 0.:
            self.tcharge = 24.
        else:
            self.tcharge = -24.
        self.amp_hrs_remaining = max(q_capacity - self.q_min + delta_q, 0.) / 3600.
        if soc > 0.:
            self.amp_hrs_remaining_ekf = self.amp_hrs_remaining * (self.soc_ekf - self.soc_min) /\
                                         max(soc - self.soc_min, 1e-8)
        else:
            self.amp_hrs_remaining_ekf = 0.
        return self.tcharge

    def construct_state_space_monitor(self):
        """ State-space representation of dynamics
        Inputs:
            ib      Current at = Vb = current at shunt, A
            voc     Voltage at storage, A
        Outputs:
            vb      Voltage at terminal, V
        States:
            vbc     RC vb-vc, V
            vcd     RC vc-vd, V
        Other:
            ioc = ib     Charge current, A
            vc      Voltage downstream of charge transfer model, ct-->c
            vd      Voltage downstream of diffusion process model, dif-->d
        """
        c_ct = self.tau_ct / self.rct
        c_dif = self.tau_dif / self.r_dif
        a = np.array([[-1 / self.tau_ct, 0],
                      [0, -1 / self.tau_dif]])
        b = np.array([[1 / c_ct,   0],
                      [1 / c_dif,  0]])
        c = np.array([-1., -1])
        d = np.array([-self.r0, 1])
        return a, b, c, d

    def count_coulombs(self, dt=0., reset=False, temp_c=25., charge_curr=0., sat=True, t_last=0.):
        raise NotImplementedError

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

    def init_battery(self):
        self.Randles.init_state_space([0., 0.])

    def look(self, temp_c):
        # Table lookups of Zhang coefficients
        b = self.lut_b.lookup(T_degC=temp_c)
        a = self.lut_a.lookup(T_degC=temp_c)
        c = self.lut_c.lookup(T_degC=temp_c)
        return b, a, c


class BatteryModel(Battery):
    """Extend basic monitoring class to run a model"""

    def __init__(self, t_t=None, t_b=None, t_a=None, t_c=None, m=0.478, n=0.4, d=0.707, num_cells=4, bat_v_sat=3.4625,
                 q_cap_rated=RATED_BATT_CAP*3600, t_rated=RATED_TEMP, t_rlim=0.017):

        Battery.__init__(self, t_t, t_b, t_a, t_c, m, n, d, num_cells, bat_v_sat, q_cap_rated,
                         t_rated, t_rlim)
        self.sat_ib_max = 0.  # Current cutback to be applied to modeled ib output, A
        self.sat_ib_null = 0.  # Current cutback value for voc=vsat, A
        self.sat_cutback_gain = 1.  # Gain to retard ib when voc exceeds vsat, dimensionless
        self.model_cutback = True  # Indicate that modeled current being limited on saturation cutback,
        # T = cutback limited
        self.model_saturated = True  # Indicator of maximal cutback, T = cutback saturated
        self.ib_sat = 0.  # Threshold to declare saturation.  This regeneratively slows down charging so if too
        # small takes too long, A
        self.s_cap = 1.  # Rated capacity scalar
        self.Randles.A, self.Randles.B, self.Randles.C, self.Randles.D = self.construct_state_space_model()

    def calculate(self, temp_c=0., soc=0., curr_in=0., dt=0., q_capacity=0., q_cap=0.):
        self.dt = dt
        self.temp_c = temp_c

        soc_lim = max(min(soc, mxeps_bb), mneps_bb)
        # SOC = soc * q_capacity / self.q_cap_rated_scaled * 100.

        # VOC - OCV model
        b, a, c, log_soc, exp_n_soc, pow_log_soc = self.calc_soc_voc_coeff(soc_lim, temp_c)
        self.voc, self.dv_dsoc = self.calc_soc_voc(soc_lim, b, a, c, log_soc, exp_n_soc, pow_log_soc)
        self.voc = min(self.voc + (soc - soc_lim) * self.dv_dsoc, max_voc)  # slightly beyond but don't windup
        self.voc += self.dv  # Experimentally varied

        # Dynamic emf
        # Randles dynamic model for model, reverse version to generate sensor inputs {ib, voc} --> {vb}, ioc=ib
        u = [self.ib, self.voc]
        self.Randles.calc_x_dot(u)
        self.Randles.update(dt)
        self.vb = self.Randles.y[0]
        self.vdyn = self.vb - self.voc

        # Saturation logic, both full and empty
        self.vsat = self.nom_vsat + (temp_c - 25.) * self.dvoc_dt
        self.sat_ib_max = self.sat_ib_null + (self.vsat - self.voc) / self.nom_vsat * q_capacity / 3600. *\
            self.sat_cutback_gain * rp.cutback_gain_scalar
        self.ib = min(curr_in, self.sat_ib_max)
        if (self.q <= 0.) & (curr_in < 0.):
            self.ib = 0.  # empty
        self.model_cutback = (self.voc > self.vsat) & (self.ib == self.sat_ib_max)
        self.model_saturated = (self.voc > self.vsat) & (self.ib < self.ib_sat) & (self.ib == self.sat_ib_max)
        Coulombs.sat = self.model_saturated

        return self.vb

    def construct_state_space_model(self):
        """ State-space representation of dynamics
        Inputs:
            ib      Current at = Vb = current at shunt, A
            voc     Voltage at storage, A
        Outputs:
            vb      Voltage at terminal, V
        States:
            vbc     RC vb-vc, V
            vcd     RC vc-vd, V
        Other:
            ioc = ib     Voltage at positive, V
            vc      Voltage downstream of charge transfer model, ct-->c
            vd      Voltage downstream of diffusion process model, dif-->d
        """
        c_ct = self.tau_ct / self.rct
        c_dif = self.tau_dif / self.r_dif
        a = np.array([[-1 / self.tau_ct, 0],
                      [0, -1 / self.tau_dif]])
        b = np.array([[1 / c_ct,   0],
                      [1 / c_dif,  0]])
        c = np.array([-1., -1])
        d = np.array([-self.r0, 1])
        return a, b, c, d

    def count_coulombs(self, dt=0., reset=False, temp_c=25., charge_curr=0., sat=True, t_last=0.):
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
        if sat:
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


# Other functions
def is_sat(temp_c, voc):
    vsat = sat_voc(temp_c)
    return voc >= vsat


def calculate_capacity(temp_c, t_sat, q_sat):
    return q_sat * (1-DQDT*(temp_c - t_sat))


def calculate_saturation_charge(t_sat, q_cap):
    return q_cap * ((t_sat - 25.)*DQDT + 1.)


def calc_vsat(temp_c):
    return sat_voc(temp_c)


def sat_voc(temp_c):
    return batt_vsat + (temp_c-25.)*BATT_DVOC_DT
