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
from Hysteresis import Hysteresis
import matplotlib.pyplot as plt


class Retained:

    def __init__(self):
        self.cutback_gain_scalar = 1.
        self.delta_q = 0.
        self.t_last = 25.
        self.delta_q_model = 0.
        self.t_last_model = 25.
        self.modeling = True


rp = Retained()


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.ib = []
        self.vb = []
        self.vc = []
        self.vd = []
        self.voc = []
        self.vbc = []
        self.vcd = []
        self.icc = []
        self.irc = []
        self.icd = []
        self.ird = []
        self.vcd_dot = []
        self.vbc_dot = []
        self.soc = []
        self.SOC = []
        self.pow_oc = []
        self.soc_ekf = []
        self.SOC_ekf = []
        self.voc_dyn = []
        self.Fx = []
        self.Bu = []
        self.P = []
        self.H = []
        self.S = []
        self.K = []
        self.hx = []
        self.u_kf = []
        self.x_kf = []
        self.y_kf = []
        self.z_ekf = []
        self.x_prior = []
        self.P_prior = []
        self.x_post = []
        self.P_post = []
        self.e_soc_ekf = []
        self.e_voc_ekf = []


# Battery constants
# RATED_BATT_CAP = 100.
# """Nominal battery bank capacity, Ah(100).Accounts for internal losses.This is
#                         what gets delivered, e.g.Wshunt / NOM_SYS_VOLT.  Also varies 0.2 - 0.4 C currents
#                         or 20 - 40 A for a 100 Ah battery"""
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


class Battery(Coulombs, EKF_1x1, Hysteresis):
    # Battery model:  Randle's dynamics, SOC-VOC model

    RATED_BATT_CAP = 100.
    RATED_TEMP = 25.
    """Nominal battery bank capacity, Ah(100).Accounts for internal losses.This is
                            what gets delivered, e.g.Wshunt / NOM_SYS_VOLT.  Also varies 0.2 - 0.4 C currents
                            or 20 - 40 A for a 100 Ah battery"""

    def __init__(self, t_t=None, t_b=None, t_a=None, t_c=None, m=0.478, n=0.4, d=0.707,
                 num_cells=4, bat_v_sat=3.4625, q_cap_rated=RATED_BATT_CAP*3600, t_rated=RATED_TEMP, t_rlim=0.017,
                 r_sd=70., tau_sd=1.8e7, r0=0.003, tau_ct=0.2, r_ct=0.0016, tau_dif=83., r_dif=0.0077,
                 temp_c=RATED_TEMP):
        """ Default values from Taborelli & Onori, 2013, State of Charge Estimation Using Extended Kalman Filters for
        Battery Management System.   Battery equations from LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang
        etal.pdf.'  SOC-OCV curve fit './Battery State/BattleBorn Rev1.xls:Model Fit' using solver with min slope
        constraint >=0.02 V/soc.  m and n using Zhang values.   Had to scale soc because  actual capacity > NOM_BAT_CAP
        so equations error when soc<=0 to match data.    See Battery.h
        """
        # Parents
        Coulombs.__init__(self, q_cap_rated,  q_cap_rated, t_rated, t_rlim)
        EKF_1x1.__init__(self)

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
        self.pow_oc = 0.  # Charge power, W
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
        self.r_sd = r_sd
        self.tau_sd = tau_sd
        self.r0 = r0
        self.tau_ct = tau_ct
        self.r_ct = float(r_ct)
        self.c_ct = self.tau_ct / self.r_ct
        self.tau_dif = tau_dif
        self.r_dif = r_dif
        self.c_dif = self.tau_dif / self.r_dif
        self.Randles = StateSpace(2, 2, 1)
        self.Randles.A, self.Randles.B, self.Randles.C, self.Randles.D = self.construct_state_space_monitor()
        self.temp_c = temp_c
        self.tcharge_ekf = 0.  # Charging time to 100% from ekf, hr
        self.voc_dyn = 0.  # Charging voltage, V
        self.soc_ekf = 0.  # Filtered state of charge from ekf (0-1)
        self.SOC_ekf = 0.  # Filtered state of charge from ekf (0-100)
        self.q_ekf = 0  # Filtered charge calculated by ekf, C
        self.amp_hrs_remaining = 0  # Discharge amp*time left if drain to q=0, A-h
        self.amp_hrs_remaining_ekf = 0  # Discharge amp*time left if drain to q_ekf=0, A-h
        self.saved = Saved()  # for plots and prints
        self.e_soc_ekf = 0.  # analysis parameter
        self.e_voc_ekf = 0.  # analysis parameter
        self.Q = 0.001*0.001
        self.R = 0.1*0.1

    def __str__(self):
        """Returns representation of the object"""
        s = "Battery:  "
        s += 'temp, #cells, b, a, c, m, n, d, dvoc_dt = {:5.1f}, {}, {:7.3f}, {:7.3f},' \
             ' {:7.3f}, {:7.3f}, {:7.3f}, {:7.3f}, {:7.3f},\n'.\
            format(self.temp_c, self.num_cells, self.b, self.a, self.c, self.m, self.n, self.d, self.dvoc_dt)
        s += 'r0, r_ct, tau_ct, r_dif, tau_dif, r_sd, tau_sd = {:7.3f}, {:7.3f}, {:7.3f},' \
             ' {:7.3f}, {:7.3f}, {:7.3f}, {:7.3f},\n'.\
            format(self.r0, self.r_ct, self.tau_ct, self.r_dif, self.tau_dif, self.r_sd, self.tau_sd)
        s += "  dv_dsoc = {:7.3f}  // Derivative scaled, V/fraction\n".format(self.dv_dsoc)
        s += "  ib =      {:7.3f}  // Current into battery, A\n".format(self.ib)
        s += "  vb =      {:7.3f}  // Total model voltage, voltage at terminals, V\n".format(self.vb)
        s += "  voc =     {:7.3f}  // Static model open circuit voltage, V\n".format(self.voc)
        s += "  vsat =    {:7.3f}  // Saturation threshold at temperature, V\n".format(self.vsat)
        s += "  voc_dyn = {:7.3f}  // Charging voltage, V\n".format(self.voc_dyn)
        s += "  vdyn =    {:7.3f}  // Model current induced back emf, V\n".format(self.vdyn)
        s += "  q =       {:7.3f}  // Present charge, C\n".format(self.q)
        s += "  q_ekf     {:7.3f}  // Filtered charge calculated by ekf, C\n".format(self.q_ekf)
        s += "  tcharge = {:7.3f}  // Charging time to full, hr\n".format(self.tcharge)
        s += "  tcharge_ekf = {:7.3f}   // Charging time to full from ekf, hr\n".format(self.tcharge_ekf)
        s += "  soc_ekf = {:7.3f}  // Filtered state of charge from ekf (0-1)\n".format(self.soc_ekf)
        s += "  SOC_ekf  ={:7.3f}  // Filtered state of charge from ekf (0-100)\n".format(self.SOC_ekf)
        s += "  amp_hrs_remaining =       {:7.3f}  // Discharge amp*time left if drain to q=0, A-h\n".\
            format(self.amp_hrs_remaining,)
        s += "  amp_hrs_remaining_ekf_ =  {:7.3f}  // Discharge amp*time left if drain to q_ekf=0, A-h\n".\
            format(self.amp_hrs_remaining_ekf)
        s += "  sr =      {:7.3f}  // Resistance scalar\n".format(self.sr)
        s += "  dv_ =     {:7.3f}  / Adjustment, V\n".format(self.dv)
        s += "  dt_ =     {:7.3f}  // Update time, s\n".format(self.dt)
        s += "\n"
        s += Coulombs.__str__(self)
        s += "\n"
        s += self.Randles.__str__()
        s += "\n"
        s += EKF_1x1.__str__(self)
        s += "\n"
        return s

    def assign_temp_c(self, temp_c):
        self.temp_c = temp_c

    def assign_soc(self, soc, voc):
        self.soc = soc
        self.voc = voc
        self.vsat = self.nom_vsat + (self.temp_c - RATED_TEMP) * self.dvoc_dt
        self.sat = self.voc >= self.vsat

    def calc_h_jacobian(self, soc_lim=0., b=0., c=0., log_soc=0., exp_n_soc=0., pow_log_soc=0.):
        dv_dsoc = float(self.num_cells) * (b * self.m / soc_lim * pow_log_soc / log_soc + c +
                                           self.d * self.n * exp_n_soc)
        return dv_dsoc

    def calc_soc_voc(self, soc_lim, b, a, c, log_soc, exp_n_soc, pow_log_soc):
        """SOC-OCV curve fit method per Zhang, et al """
        dv_dsoc = self.calc_h_jacobian(soc_lim, b, c, log_soc, exp_n_soc, pow_log_soc)
        voc = self.num_cells * (a + b * pow_log_soc + c * soc_lim + self.d * exp_n_soc)
        return voc, dv_dsoc

    def calc_soc_voc_coeff(self, soc, tc, n, m):
        """SOC-OCV curve fit method per Zhang, et al """
        # Zhang coefficients
        b, a, c = self.look(tc)
        log_soc_norm = math.log(soc)
        exp_n_soc_norm = math.exp(n * (soc - 1))
        pow_log_soc_norm = math.pow(-log_soc_norm, m)
        return b, a, c, log_soc_norm, exp_n_soc_norm, pow_log_soc_norm

    def calculate(self, temp_c, soc, curr_in, dt, q_capacity):
        raise NotImplementedError

    def calculate_ekf(self, temp_c, vb, ib, dt):
        self.temp_c = temp_c
        self.vsat = calc_vsat(self.temp_c)

        # Dynamics
        self.vb = vb
        self.ib = ib
        u = np.array([ib, vb]).T
        self.Randles.calc_x_dot(u)
        self.Randles.update(dt)
        self.voc_dyn = self.Randles.y
        self.vdyn = self.vb - self.voc_dyn
        self.voc = self.voc_dyn
        self.pow_oc = self.voc * self.ib

        # EKF 1x1
        self.predict_ekf(ib)  # u = ib
        self.update_ekf(self.voc_dyn, mneps_bb, mxeps_bb)  # z = voc_dyn, voc_filtered = hx
        self.soc_ekf = self.x_kf  # x = Vsoc (0-1 ideal capacitor voltage) proxy for soc
        self.q_ekf = self.soc_ekf * self.q_capacity
        self.SOC_ekf = self.q_ekf / self.q_cap_rated_scaled * 100.

        # Charge time
        if self.ib > 0.1:
            self.tcharge_ekf = min(Battery.RATED_BATT_CAP/self.ib * (1. - self.soc_ekf), 24.)
        elif self.ib < -0.1:
            self.tcharge_ekf = max(Battery.RATED_BATT_CAP/self.ib * self.soc_ekf, -24.)
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
            vb      Voltage at terminal, V
        Outputs:
            voc     Voltage at storage, A
        States:
            vbc     RC vb-vc, V
            vcd     RC vc-vd, V
        Other:
            ioc = ib     Charge current, A
            vc      Voltage downstream of charge transfer model, ct-->c
            vd      Voltage downstream of diffusion process model, dif-->d
        """
        a = np.array([[-1 / self.tau_ct, 0],
                      [0, -1 / self.tau_dif]])
        b = np.array([[1 / self.c_ct,   0],
                      [1 / self.c_dif,  0]])
        c = np.array([-1., -1])
        d = np.array([-self.r0, 1])
        return a, b, c, d

    # def count_coulombs(self, dt=0., reset=False, temp_c=25., charge_curr=0., sat=True, t_last=0.):
    #     raise NotImplementedError

    def ekf_model_predict(self):
        """Process model"""
        self.Fx = math.exp(-self.dt / self.tau_sd)
        self.Bu = (1. - self.Fx)*self.r_sd
        return self.Fx, self.Bu

    def ekf_model_update(self):
        # Measurement function hx(x), x = soc ideal capacitor
        x_lim = max(min(self.x_kf, mxeps_bb), mneps_bb)
        self.b, self.a, self.c, log_soc, exp_n_soc, pow_log_soc =\
            self.calc_soc_voc_coeff(x_lim, self.temp_c, self.n, self.m)
        self.hx, self.dv_dsoc = self.calc_soc_voc(x_lim, self.b, self.a, self.c, log_soc, exp_n_soc, pow_log_soc)
        # Jacobian of measurement function
        self.H = self.dv_dsoc
        return self.hx, self.H

    def init_battery(self):
        self.Randles.init_state_space([0., 0.])

    def init_soc_ekf(self, soc):
        self.soc_ekf = soc
        self.init_ekf(soc, 0.0)
        self.q_ekf = self.soc_ekf * self.q_capacity
        self.SOC_ekf = self.q_ekf / self.q_cap_rated_scaled * 100.

    def look(self, temp_c):
        # Table lookups of Zhang coefficients
        b = self.lut_b.lookup(T_degC=temp_c)
        a = self.lut_a.lookup(T_degC=temp_c)
        c = self.lut_c.lookup(T_degC=temp_c)
        return b, a, c

    def vbc(self):
        return self.Randles.x[0]

    def vcd(self):
        return self.Randles.x[1]

    def vbc_dot(self):
        return self.Randles.x_dot[0]

    def vcd_dot(self):
        return self.Randles.x_dot[1]

    def i_c_ct(self):
        return self.vbc_dot() * self.c_ct

    def i_c_dif(self):
        return self.vcd_dot() * self.c_dif

    def i_r_ct(self):
        return self.vbc() / self.r_ct

    def i_r_dif(self):
        return self.vcd() / self.r_dif

    def vd(self):
        return self.voc + self.ib * self.r0

    def vc(self):
        return self.vd() + self.vcd()

    def save(self, time, soc_ref, voc_ref):
        self.saved.time.append(time)
        self.saved.ib.append(self.ib)
        self.saved.vb.append(self.vb)
        self.saved.vc.append(self.vc())
        self.saved.vd.append(self.vd())
        self.saved.voc.append(self.voc)
        self.saved.vbc.append(self.vbc())
        self.saved.vcd.append(self.vcd())
        self.saved.icc.append(self.i_c_ct())
        self.saved.irc.append(self.i_r_ct())
        self.saved.icd.append(self.i_c_dif())
        self.saved.ird.append(self.i_r_dif())
        self.saved.vcd_dot.append(self.vcd_dot())
        self.saved.vbc_dot.append(self.vbc_dot())
        self.saved.soc.append(self.soc)
        self.saved.SOC.append(self.SOC)
        self.saved.pow_oc.append(self.pow_oc)
        self.saved.soc_ekf.append(self.soc_ekf)
        self.saved.SOC_ekf.append(self.SOC_ekf)
        self.saved.voc_dyn.append(self.voc_dyn)
        self.saved.Fx.append(self.Fx)
        self.saved.Bu.append(self.Bu)
        self.saved.P.append(self.P)
        self.saved.H.append(self.H)
        self.saved.S.append(self.S)
        self.saved.K.append(self.K)
        self.saved.hx.append(self.hx)
        self.saved.u_kf.append(self.u_kf)
        self.saved.x_kf.append(self.x_kf)
        self.saved.y_kf.append(self.y_kf)
        self.saved.z_ekf.append(self.z_ekf)
        self.saved.x_prior.append(self.x_prior)
        self.saved.P_prior.append(self.P_prior)
        self.saved.x_post.append(self.x_post)
        self.saved.P_post.append(self.P_post)
        self.e_soc_ekf = (self.soc_ekf - soc_ref) / soc_ref
        self.e_voc_ekf = (self.voc_dyn - voc_ref) / voc_ref
        self.saved.e_soc_ekf.append(self.e_soc_ekf)
        self.saved.e_voc_ekf.append(self.e_voc_ekf)


class BatteryModel(Battery):
    """Extend basic monitoring class to run a model"""

    def __init__(self, t_t=None, t_b=None, t_a=None, t_c=None, m=0.478, n=0.4, d=0.707,
                 num_cells=4, bat_v_sat=3.4625, q_cap_rated=Battery.RATED_BATT_CAP * 3600,
                 t_rated=Battery.RATED_TEMP, t_rlim=0.017, scale=1.,
                 r_sd=70., tau_sd=1.8e7, r0=0.003, tau_ct=0.2, r_ct=0.0016, tau_dif=83., r_dif=0.0077,
                 temp_c=Battery.RATED_TEMP):
        Battery.__init__(self, t_t, t_b, t_a, t_c, m, n, d, num_cells, bat_v_sat, q_cap_rated, t_rated,
                         t_rlim, r_sd, tau_sd, r0, tau_ct, r_ct, tau_dif, r_dif, temp_c)
        self.sat_ib_max = 0.  # Current cutback to be applied to modeled ib output, A
        # self.sat_ib_null = 0.1*Battery.RATED_BATT_CAP  # Current cutback value for voc=vsat, A
        self.sat_ib_null = 0.  # Current cutback value for soc=1, A
        # self.sat_cutback_gain = 4.8  # Gain to retard ib when voc exceeds vsat, dimensionless
        self.sat_cutback_gain = 1000.  # Gain to retard ib when soc approaches 1, dimensionless
        self.model_cutback = False  # Indicate that modeled current being limited on saturation cutback,
        # T = cutback limited
        self.model_saturated = False  # Indicator of maximal cutback, T = cutback saturated
        self.ib_sat = 0.5  # Threshold to declare saturation.  This regeneratively slows down charging so if too
        # small takes too long, A
        self.Randles.A, self.Randles.B, self.Randles.C, self.Randles.D = self.construct_state_space_model()
        self.s_cap = scale  # Rated capacity scalar
        if scale is not None:
            self.apply_cap_scale(scale)

    def __str__(self):
        """Returns representation of the object"""
        s = "BatteryModel:  "
        s += Battery.__str__(self)
        s += "  sat_ib_max =      {:7.3f}  // Current cutback to be applied to modeled ib output, A\n".\
            format(self.sat_ib_max)
        s += "  ib_null    =      {:7.3f}  // Current cutback value for voc=vsat, A\n".\
            format(self.sat_ib_null)
        s += "  sat_cutback_gain = {:6.2f}  // Gain to retard ib when voc exceeds vsat, dimensionless\n".\
            format(self.sat_cutback_gain)
        s += "  model_cutback =         {:d}  // Indicate that modeled current being limited on" \
             " saturation cutback, T = cutback limited\n".format(self.model_cutback)
        s += "  model_saturated =       {:d}  // Indicator of maximal cutback, T = cutback saturated\n".\
            format(self.model_saturated)
        s += "  ib_sat =          {:7.3f}  // Threshold to declare saturation.  This regeneratively slows" \
             " down charging so if too\n".format(self.ib_sat)
        s += "\n"
        return s

    def calculate(self, temp_c, soc, curr_in, dt, q_capacity):
        self.dt = dt
        self.temp_c = temp_c

        soc_lim = max(min(soc, mxeps_bb), mneps_bb)
        # SOC = soc * q_capacity / self.q_cap_rated_scaled * 100.

        # VOC - OCV model
        self.b, self.a, self.c, log_soc, exp_n_soc, pow_log_soc =\
            self.calc_soc_voc_coeff(soc_lim, temp_c, self.n, self.m)
        self.voc, self.dv_dsoc = self.calc_soc_voc(soc_lim, self.b, self.a, self.c, log_soc, exp_n_soc, pow_log_soc)
        self.voc = min(self.voc + (soc - soc_lim) * self.dv_dsoc, max_voc)  # slightly beyond but don't windup
        self.voc += self.dv  # Experimentally varied

        # Dynamic emf
        # Randles dynamic model for model, reverse version to generate sensor inputs {ib, voc} --> {vb}, ioc=ib
        u = np.array([self.ib, self.voc]).T
        self.Randles.calc_x_dot(u)
        self.Randles.update(dt)
        self.vb = self.Randles.y
        self.vdyn = self.vb - self.voc

        # Saturation logic, both full and empty
        self.vsat = self.nom_vsat + (temp_c - 25.) * self.dvoc_dt
        self.sat_ib_max = self.sat_ib_null + (1 - self.soc) * self.sat_cutback_gain * rp.cutback_gain_scalar
        self.ib = min(curr_in, self.sat_ib_max)
        if (self.q <= 0.) & (curr_in < 0.):
            self.ib = 0.  # empty
        self.model_cutback = (self.voc > self.vsat) & (self.ib == self.sat_ib_max)
        self.model_saturated = (self.voc > self.vsat) & (self.ib < self.ib_sat) & (self.ib == self.sat_ib_max)
        Coulombs.sat = self.model_saturated
        self.pow_oc = self.voc * self.ib

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
        a = np.array([[-1 / self.tau_ct, 0],
                      [0, -1 / self.tau_dif]])
        b = np.array([[1 / self.c_ct,   0],
                      [1 / self.c_dif,  0]])
        c = np.array([1., 1])
        d = np.array([self.r0, 1])
        return a, b, c, d

    def count_coulombs(self, dt, reset, temp_c, charge_curr, sat, t_last):
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


def overall(ms, ss, filename, fig_files=None, plot_title=None, n_fig=None, ref=None):
    if fig_files is None:
        fig_files = []
    if ref is None:
        ref = []

    plt.figure()
    n_fig += 1
    plt.subplot(321)
    plt.title(plot_title)
    # plt.plot(ms.time, ref, color='black', label='curr dmd, A')
    plt.plot(ms.time, ms.ib, color='green', label='ib')
    plt.plot(ms.time, ms.irc, color='red', label='I_R_ct')
    plt.plot(ms.time, ms.icd, color='cyan', label='I_C_dif')
    plt.plot(ms.time, ms.ird, color='orange', linestyle='--', label='I_R_dif')
    # plt.plot(ms.time, ms.ib, color='black', linestyle='--', label='Ioc')
    plt.legend(loc=1)
    plt.subplot(323)
    plt.plot(ms.time, ms.vb, color='green', label='Vb')
    plt.plot(ms.time, ms.vc, color='blue', label='Vc')
    plt.plot(ms.time, ms.vd, color='red', label='Vd')
    plt.plot(ms.time, ms.voc_dyn, color='orange', label='Voc_dyn')
    plt.legend(loc=1)
    plt.subplot(325)
    plt.plot(ms.time, ms.vbc_dot, color='green', label='Vbc_dot')
    plt.plot(ms.time, ms.vcd_dot, color='blue', label='Vcd_dot')
    plt.legend(loc=1)
    plt.subplot(322)
    plt.plot(ms.time, ms.soc, color='red', label='soc')
    plt.legend(loc=1)
    plt.subplot(324)
    plt.plot(ms.time, ms.pow_oc, color='orange', label='Pow_charge')
    plt.legend(loc=1)
    plt.subplot(326)
    plt.plot(ms.soc, ms.voc, color='black', label='voc vs soc')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.subplot(321)
    plt.title(plot_title+' **SIM')
    # plt.plot(ss.time, ref, color='black', label='curr dmd, A')
    plt.plot(ss.time, ss.ib, color='green', label='ib')
    plt.plot(ss.time, ss.irc, color='red', label='I_R_ct')
    plt.plot(ss.time, ss.icd, color='cyan', label='I_C_dif')
    plt.plot(ss.time, ss.ird, color='orange', linestyle='--', label='I_R_dif')
    # plt.plot(ss.time, ss.ib, color='black', linestyle='--', label='Ioc')
    plt.legend(loc=1)
    plt.subplot(323)
    plt.plot(ss.time, ss.vb, color='green', label='Vb')
    plt.plot(ss.time, ss.vc, color='blue', label='Vc')
    plt.plot(ss.time, ss.vd, color='red', label='Vd')
    plt.plot(ss.time, ss.voc, color='orange', label='Voc')
    plt.legend(loc=1)
    plt.subplot(325)
    plt.plot(ss.time, ss.vbc_dot, color='green', label='Vbc_dot')
    plt.plot(ss.time, ss.vcd_dot, color='blue', label='Vcd_dot')
    plt.legend(loc=1)
    plt.subplot(322)
    plt.plot(ss.time, ss.soc, color='red', label='soc')
    plt.legend(loc=1)
    plt.subplot(324)
    plt.plot(ss.time, ss.pow_oc, color='orange', label='Pow_charge')
    plt.legend(loc=1)
    plt.subplot(326)
    plt.plot(ss.soc, ss.voc, color='black', label='voc vs soc')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.subplot(331)
    plt.title(plot_title+' **EKF')
    plt.plot(ms.time, ms.x_kf, color='red', linestyle='dotted', label='x ekf')
    plt.legend(loc=4)
    plt.subplot(332)
    plt.plot(ms.time, ms.hx, color='cyan', linestyle='dotted', label='hx ekf')
    plt.plot(ms.time, ms.z_ekf, color='black', linestyle='dotted', label='z ekf')
    plt.legend(loc=4)
    plt.subplot(333)
    plt.plot(ms.time, ms.y_kf, color='green', linestyle='dotted', label='y ekf')
    plt.legend(loc=4)
    plt.subplot(334)
    plt.plot(ms.time, ms.H, color='magenta', linestyle='dotted', label='H ekf')
    plt.ylim(0, 150)
    plt.legend(loc=3)
    plt.subplot(335)
    plt.plot(ms.time, ms.P, color='orange', linestyle='dotted', label='P ekf')
    plt.legend(loc=3)
    plt.subplot(336)
    plt.plot(ms.time, ms.Fx, color='red', linestyle='dotted', label='Fx ekf')
    plt.legend(loc=2)
    plt.subplot(337)
    plt.plot(ms.time, ms.Bu, color='blue', linestyle='dotted', label='Bu ekf')
    plt.legend(loc=2)
    plt.subplot(338)
    plt.plot(ms.time, ms.K, color='red', linestyle='dotted', label='K ekf')
    plt.legend(loc=4)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.title(plot_title)
    plt.plot(ms.time, ms.e_voc_ekf, color='blue', linestyle='dotted', label='e_voc')
    plt.plot(ms.time, ms.e_soc_ekf, color='red', linestyle='dotted', label='e_soc_ekf')
    plt.ylim(-0.01, 0.01)
    plt.legend(loc=2)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.title(plot_title)
    plt.plot(ss.time, ss.voc, color='green', label='voc model')
    plt.plot(ms.time, ms.voc_dyn, color='red', linestyle='dotted', label='voc dyn est')
    plt.plot(ms.time, ms.voc, color='blue', linestyle='dotted', label='voc ekf')
    plt.legend(loc=4)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.title(plot_title)
    plt.plot(ss.time, ss.soc, color='green', label='soc model')
    plt.plot(ms.time, ms.soc, color='red', linestyle='dotted', label='soc counted')
    plt.plot(ms.time, ms.soc_ekf, color='blue', linestyle='dotted', label='soc ekf')
    plt.legend(loc=4)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.title(plot_title)
    plt.plot(ms.time, ms.e_voc_ekf, color='blue', linestyle='dotted', label='e_voc')
    plt.plot(ms.time, ms.e_soc_ekf, color='red', linestyle='dotted', label='e_soc_ekf')
    plt.legend(loc=2)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files
