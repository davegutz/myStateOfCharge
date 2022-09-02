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

"""Define a general purpose battery model including Randles' model and SoC-VOV model."""

import numpy as np
import math
from EKF1x1 import EKF1x1
from Coulombs import Coulombs, dqdt
from StateSpace import StateSpace
from Hysteresis import Hysteresis
import matplotlib.pyplot as plt
from TFDelay import TFDelay
from myFilters import LagTustin, General2Pole, RateLimit
from Scale import Scale


class Retained:

    def __init__(self):
        self.cutback_gain_scalar = 1.
        self.delta_q = 0.
        self.t_last = 25.
        self.delta_q_model = 0.
        self.t_last_model = 25.
        self.modeling = 7  # assumed for this 'model'; over-ridden later
        self.nS = 1  # assumed for this 'model'
        self.nP = 1  # assumed for this 'model'

    def tweak_test(self):
        return 0b001 & int(self.modeling)


# Battery constants
RATED_TEMP = 25.  # Temperature at RATED_BATT_CAP, deg C
BATT_DVOC_DT = 0.004  # 5/30/2022
""" Change of VOC with operating temperature in range 0 - 50 C (0.004) V/deg C
                            >3.425 V is reliable approximation for SOC>99.7 observed in my prototype around 15-35 C"""
BATT_V_SAT = 13.8  # Normal battery cell saturation for SOC=99.7, V (13.8)
NOM_SYS_VOLT = 12.  # Nominal system output, V, at which the reported amps are used (12)
low_voc = 10  # Minimum voltage for battery below which BMS shutsoff current
low_t = 8  # Minimum temperature for valid saturation check, because BMS shuts off battery low.
# Heater should keep >8, too
mxeps_bb = 1-1e-6  # Numerical maximum of coefficient model with scaled soc
mneps_bb = 1e-6  # Numerical minimum of coefficient model without scaled soc
# DQDT = 0.01  # Change of charge with temperature, fraction/deg C.  From literature.  0.01 is commonly used
CAP_DROOP_C = 20.  # Temperature below which a floor on q arises, C (20)
TCHARGE_DISPLAY_DEADBAND = 0.1  # Inside this +/- deadband, charge time is displayed '---', A
max_voc = 1.2*NOM_SYS_VOLT  # Prevent windup of battery model, V
batt_vsat = BATT_V_SAT  # Total bank saturation for 0.997=soc, V
batt_vmax = 14.3  # Observed max voltage of 14.3 V at 25C for 12V prototype bank, V
DF1 = 0.02  # Weighted selection lower transition drift, fraction
DF2 = 0.70  # Threshold to reset Coulomb Counter if different from ekf, fraction (0.05)
EKF_CONV = 2e-3  # EKF tracking error indicating convergence, V (1e-3)
EKF_T_CONV = 30.  # EKF set convergence test time, sec (30.)
EKF_T_RESET = (EKF_T_CONV/2.)  # EKF reset test time, sec ('up 1, down 2')
EKF_NOM_DT = 0.1  # EKF nominal update time, s (initialization; actual value varies)
TAU_Y_FILT = 5.  # EKF y-filter time constant, sec (5.)
MIN_Y_FILT = -0.5  # EKF y-filter minimum, V (-0.5)
MAX_Y_FILT = 0.5  # EKF y-filter maximum, V (0.5)
WN_Y_FILT = 0.1  # EKF y-filter-2 natural frequency, r/s (0.1)
ZETA_Y_FILT = 0.9  # EKF y-filter-2 damping factor (0.9)
TMAX_FILT = 3.  # Maximum y-filter-2 sample time, s (3.)
EKF_Q_SD_NORM = 0.00005  # Standard deviation of normal EKF process uncertainty, V (0.00005)
EKF_R_SD_NORM = 0.5  # Standard deviation of normal EKF state uncertainty, fraction (0-1) (0.5)
# disable because not in particle photon logic
# EKF_Q_SD_REV = 0.7
# EKF_R_SD_REV = 0.3
EKF_Q_SD_REV = EKF_Q_SD_NORM
EKF_R_SD_REV = EKF_R_SD_NORM
IMAX_NUM = 100000.

class Battery(Coulombs):
    RATED_BATT_CAP = 100.
    # """Nominal battery bank capacity, Ah(100).Accounts for internal losses.This is
    #                         what gets delivered, e.g.Wshunt / NOM_SYS_VOLT.  Also varies 0.2 - 0.4 C currents
    #                         or 20 - 40 A for a 100 Ah battery"""

    # Battery model:  Randles' dynamics, SOC-VOC model

    """Nominal battery bank capacity, Ah(100).Accounts for internal losses.This is
                            what gets delivered, e.g.Wshunt / NOM_SYS_VOLT.  Also varies 0.2 - 0.4 C currents
                            or 20 - 40 A for a 100 Ah battery"""

    def __init__(self, bat_v_sat=13.8, q_cap_rated=RATED_BATT_CAP*3600, t_rated=RATED_TEMP, t_rlim=0.017,
                 r_sd=70., tau_sd=2.5e7, r0=0.003, tau_ct=0.2, r_ct=0.0016, tau_dif=83., r_dif=0.0077,
                 temp_c=RATED_TEMP, tweak_test=False, t_max=0.31, sres=1.):
        """ Default values from Taborelli & Onori, 2013, State of Charge Estimation Using Extended Kalman Filters for
        Battery Management System.   Battery equations from LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang
        etal.pdf.'  SOC-OCV curve fit './Battery State/BattleBorn Rev1.xls:Model Fit' using solver with min slope
        constraint >=0.02 V/soc.  m and n using Zhang values.   Had to scale soc because  actual capacity > NOM_BAT_CAP
        so equation error when soc<=0 to match data.    See Battery.h
        """
        # Parents
        Coulombs.__init__(self, q_cap_rated,  q_cap_rated, t_rated, t_rlim, tweak_test)

        # Defaults
        from pyDAGx import myTables
        t_x_soc = [0.00, 0.05, 0.10, 0.14, 0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99,  0.995, 1.00]
        t_y_t = [5.,  11.1,  20.,   40.]
        t_voc = [4.00, 4.00,  10.00, 11.80, 12.45, 12.55, 12.70, 12.77, 12.90, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.75, 14.45,
                 4.00, 8.00,  11.70, 12.50, 12.60, 12.70, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 13.76, 14.46,
                 9.00, 12.45, 12.65, 12.77, 12.85, 12.89, 12.95, 12.99, 13.03, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 13.80, 14.50,
                 9.08, 12.53, 12.73, 12.85, 12.93, 12.97, 13.03, 13.07, 13.11, 13.12, 13.17, 13.22, 13.29, 13.33, 13.35, 13.88, 14.58]

        x = np.array(t_x_soc)
        y = np.array(t_y_t)
        data_interp = np.array(t_voc)
        self.lut_voc = myTables.TableInterp2D(x, y, data_interp)
        self.nz = None
        self.q = 0  # Charge, C
        self.voc = NOM_SYS_VOLT  # Model open circuit voltage, V
        self.voc_stat = self.voc  # Static model open circuit voltage from charge process, V
        self.dv_dyn = 0.  # Model current induced back emf, V
        self.vb = NOM_SYS_VOLT  # Battery voltage at post, V
        self.ib = 0  # Current into battery post, A
        self.ioc = 0  # Current into battery process accounting for hysteresis, A
        self.dv_dsoc = 0.  # Slope of soc-voc curve, V/%
        self.tcharge = 0.  # Charging time to 100%, hr
        self.sr = 1  # Resistance scalar
        self.nom_vsat = bat_v_sat  # Normal battery cell saturation for SOC=99.7, V
        self.vsat = bat_v_sat  # Saturation voltage, V
        self.dv = 0.01  # Adjustment for voltage level, V (0.01)
        self.dvoc_dt = BATT_DVOC_DT  # Change of VOC with operating temperature in
        # range 0 - 50 C, V/deg C
        self.dt = 0  # Update time, s
        self.r_sd = r_sd
        self.tau_sd = tau_sd
        self.r0 = r0*sres
        self.tau_ct = tau_ct
        self.r_ct = float(r_ct)*sres
        self.c_ct = self.tau_ct / self.r_ct
        self.tau_dif = tau_dif
        self.r_dif = r_dif*sres
        self.c_dif = self.tau_dif / self.r_dif
        self.t_max = t_max
        self.Randles = StateSpace(2, 2, 1)
        # self.Randles.A, self.Randles.B, self.Randles.C, self.Randles.D = self.construct_state_space_monitor()
        self.temp_c = temp_c
        self.saved = Saved()  # for plots and prints
        self.dv_hys = 0.  # Placeholder so BatteryModel can be plotted
        self.dv_dyn = 0.  # Placeholder so BatteryModel can be plotted
        self.bms_off = False
        self.mod = 7
        self.sel = 0
        self.tweak_test = tweak_test
        self.r_ss = r0 + r_ct + r_dif

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix + "Battery:\n"
        s += "  temp_c  = {:7.3f}  // Battery temperature, deg C\n".format(self.temp_c)
        s += "  dvoc_dt = {:9.6f}  // Change of VOC with operating temperature in range 0 - 50 C V/deg C\n".format(self.dvoc_dt)
        s += "  r_0     = {:9.6f}  // Randles R0, ohms\n".format(self.r0)
        s += "  r_ct    = {:9.6f}  // Randles charge transfer resistance, ohms\n".format(self.r_ct)
        s += "  tau_ct  = {:9.6f}  // Randles charge transfer time constant, s (=1/Rct/Cct)\n".format(self.tau_ct)
        s += "  r_dif   = {:9.6f}  // Randles diffusion resistance, ohms\n".format(self.r_dif)
        s += "  tau_dif = {:9.6f}  // Randles diffusion time constant, s (=1/Rdif/Cdif)\n".format(self.tau_dif)
        s += "  r_ss    = {:9.6f}  // Steady state equivalent battery resistance, for solver, Ohms\n".format(self.r_ss)
        s += "  r_sd    = {:9.6f}  // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms\n".format(self.r_sd)
        s += "  tau_sd  = {:9.1f}  // Equivalent model for EKF reference.	Parasitic discharge time constant, sec\n".format(self.tau_sd)
        s += "  bms_off  = {:d}      // BMS off\n".format(self.bms_off)
        s += "  dv_dsoc = {:9.6f}  // Derivative scaled, V/fraction\n".format(self.dv_dsoc)
        s += "  ib =      {:7.3f}  // Battery terminal current, A\n".format(self.ib)
        s += "  vb =      {:7.3f}  // Battery terminal voltage, V\n".format(self.vb)
        s += "  voc      ={:7.3f}  // Static model open circuit voltage, V\n".format(self.voc)
        s += "  voc_stat ={:7.3f}  // Static model open circuit voltage from table (reference), V\n".format(self.voc_stat)
        s += "  vsat =    {:7.3f}  // Saturation threshold at temperature, V\n".format(self.vsat)
        s += "  dv_dyn =  {:7.3f}  // Current-induced back emf, V\n".format(self.dv_dyn)
        s += "  q =       {:7.3f}  // Present charge available to use, except q_min_, C\n".format(self.q)
        s += "  sr =      {:7.3f}  // Resistance scalar\n".format(self.sr)
        s += "  dv_ =     {:7.3f}  // Delta voltage, V\n".format(self.dv)
        s += "  dt_ =     {:7.3f}  // Update time, s\n".format(self.dt)
        s += "  dv_hys  = {:7.3f}  // Hysteresis delta v, V\n".format(self.dv_hys)
        s += "  tweak_test={:d}     // Driving signal injection completely using software inj_soft_bias\n".format(self.tweak_test)
        s += "\n  "
        s += Coulombs.__str__(self, prefix + 'Battery:')
        s += "\n  "
        s += self.Randles.__str__(prefix + 'Battery:')
        return s

    def assign_temp_c(self, temp_c):
        self.temp_c = temp_c

    def assign_soc(self, soc, voc):
        self.soc = soc
        self.voc = voc
        self.voc_stat = voc
        self.vsat = self.nom_vsat + (self.temp_c - RATED_TEMP) * self.dvoc_dt
        self.sat = self.voc >= self.vsat

    def calc_h_jacobian(self, soc_lim, temp_c):
        if soc_lim > 0.5:
            dv_dsoc = (self.lut_voc.interp(soc_lim, temp_c) - self.lut_voc.interp(soc_lim-0.01, temp_c)) / 0.01
        else:
            dv_dsoc = (self.lut_voc.interp(soc_lim+0.01, temp_c) - self.lut_voc.interp(soc_lim, temp_c)) / 0.01
        return dv_dsoc

    def calc_soc_voc(self, soc, temp_c):
        """SOC-OCV curve fit method per Zhang, etal """
        dv_dsoc = self.calc_h_jacobian(soc, temp_c)
        voc = self.lut_voc.interp(soc, temp_c) + self.dv
        return voc, dv_dsoc

    def calculate(self, temp_c, soc, curr_in, dt, q_capacity, dc_dc_on, reset, rp=None, sat_init=None):
        # Battery
        raise NotImplementedError

    def init_battery(self):
        self.Randles.init_state_space([0., 0.])

    def look_hys(self, dv, soc):
        raise NotImplementedError

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


class BatteryMonitor(Battery, EKF1x1):
    """Extend basic class to monitor"""

    def __init__(self, bat_v_sat=13.8, q_cap_rated=Battery.RATED_BATT_CAP*3600,
                 t_rated=RATED_TEMP, t_rlim=0.017,
                 r_sd=70., tau_sd=2.5e7, r0=0.003, tau_ct=0.2, r_ct=0.0016, tau_dif=83., r_dif=0.0077,
                 temp_c=RATED_TEMP, hys_scale=1., tweak_test=False, dv_hys=0., sres=1., scaler_q=None,
                 scaler_r=None):
        Battery.__init__(self, bat_v_sat, q_cap_rated, t_rated,
                         t_rlim, r_sd, tau_sd, r0, tau_ct, r_ct, tau_dif, r_dif, temp_c, tweak_test, sres=sres)
        self.Randles.A, self.Randles.B, self.Randles.C, self.Randles.D = self.construct_state_space_monitor()

        """ Default values from Taborelli & Onori, 2013, State of Charge Estimation Using Extended Kalman Filters for
        Battery Management System.   Battery equations from LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang
        etal.pdf.'  SOC-OCV curve fit './Battery State/BattleBorn Rev1.xls:Model Fit' using solver with min slope
        constraint >=0.02 V/soc.  m and n using Zhang values.   Had to scale soc because  actual capacity > NOM_BAT_CAP
        so equations error when soc<=0 to match data.    See Battery.h
        """
        # Parents
        EKF1x1.__init__(self)
        self.tcharge_ekf = 0.  # Charging time to 100% from ekf, hr
        self.voc = 0.  # Charging voltage, V
        self.soc_ekf = 0.  # Filtered state of charge from ekf (0-1)
        self.q_ekf = 0  # Filtered charge calculated by ekf, C
        self.amp_hrs_remaining_ekf = 0  # Discharge amp*time left if drain to q_ekf=0, A-h
        self.amp_hrs_remaining_wt = 0  # Discharge amp*time left if drain soc_wt_ to 0, A-h
        self.e_soc_ekf = 0.  # analysis parameter
        self.e_voc_ekf = 0.  # analysis parameter
        # self.Q = 0.001*0.001  # EKF process uncertainty
        # self.R = 0.1*0.1  # EKF state uncertainty
        if scaler_q is None:
            self.scaler_q = Scale(1, 4, EKF_Q_SD_REV, EKF_Q_SD_NORM)
        else:
            self.scaler_q = scaler_q
        if scaler_r is None:
            self.scaler_r = Scale(1, 4, EKF_R_SD_REV, EKF_R_SD_NORM)
        else:
            self.scaler_r = scaler_r
        self.Q = EKF_Q_SD_NORM * EKF_Q_SD_NORM  # EKF process uncertainty
        self.R = EKF_R_SD_NORM * EKF_R_SD_NORM  # EKF state uncertainty
        self.hys = Hysteresis(scale=hys_scale, dv_hys=dv_hys)  # Battery hysteresis model - drift of voc
        self.soc_m = 0.  # Model information
        self.EKF_converged = TFDelay(False, EKF_T_CONV, EKF_T_RESET, EKF_NOM_DT)
        self.y_filt_lag = LagTustin(0.1, TAU_Y_FILT, MIN_Y_FILT, MAX_Y_FILT)
        self.y_filt = 0.
        self.y_filt_2Ord = General2Pole(0.1, WN_Y_FILT, ZETA_Y_FILT, MIN_Y_FILT, MAX_Y_FILT)
        self.y_filt2 = 0.
        self.Ib = 0.
        self.Vb = 0.
        self.Voc_stat = 0.
        self.Voc = 0.
        self.Vsat = 0.
        self.dV_dyn = 0.
        self.Voc_ekf = 0.
        self.T_Rlim = RateLimit()

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix
        s += Battery.__str__(self, prefix + 'BatteryMonitor:')
        s += "  amp_hrs_remaining_ekf_ =  {:7.3f}  // Discharge amp*time left if drain to q_ekf=0, A-h\n".format(self.amp_hrs_remaining_ekf)
        s += "  amp_hrs_remaining_wt_  =  {:7.3f}  // Discharge amp*time left if drain soc_wt_ to 0, A-h\n".format(self.amp_hrs_remaining_wt)
        s += "  q_ekf     {:7.3f}  // Filtered charge calculated by ekf, C\n".format(self.q_ekf)
        s += "  soc_ekf = {:7.3f}  // Solved state of charge, fraction\n".format(self.soc_ekf)
        s += "  tcharge = {:7.3f}  // Charging time to full, hr\n".format(self.tcharge)
        s += "  tcharge_ekf = {:7.3f}   // Charging time to full from ekf, hr\n".format(self.tcharge_ekf)
        s += "  mod     =               {:f}  // Modeling\n".format(self.mod)
        s += "  \n  "
        s += self.hys.__str__(prefix + 'BatteryMonitor:')
        s += "\n  "
        s += EKF1x1.__str__(self, prefix + 'BatteryMonitor:')
        return s

    def assign_soc_m(self, soc_m):
        self.soc_m = soc_m

    def calculate(self, temp_c, vb, ib, dt, reset, q_capacity=None, dc_dc_on=None, rp=None, d_voc=None):  # BatteryMonitor
        self.temp_c = temp_c
        self.vsat = calc_vsat(self.temp_c)
        self.dt = dt
        self.mod = rp.modeling
        self.T_Rlim.update(x=self.temp_c, reset=reset, dt=dt, max_=0.017, min_=-.017)
        T_rate = self.T_Rlim.rate

        # Dynamics
        self.vb = vb
        self.ib = max(min(ib, IMAX_NUM), -IMAX_NUM)  # Overflow protection since ib past value used
        u = np.array([ib, vb]).T
        self.Randles.calc_x_dot(u)
        if dt < self.t_max:
            self.Randles.update(self.dt)
            self.voc = self.Randles.y
        else:  # aliased, unstable if update Randles
            self.voc = vb - self.r_ss * self.ib
        if d_voc:
            self.voc = d_voc
        self.dv_dyn = self.vb - self.voc
        # self.voc_stat, self.dv_dsoc = self.calc_soc_voc(self.soc, temp_c)
        # Hysteresis model
        self.hys.calculate_hys(self.ib, self.soc)
        self.dv_hys = self.hys.update(self.dt)
        # self.voc = self.voc_stat + self.dv_hys
        self.voc_stat = self.voc - self.dv_hys
        self.ioc = self.hys.ioc
        self.bms_off = self.temp_c <= low_t  # KISS
        if self.bms_off:
            self.voc_stat, self.dv_dsoc = self.calc_soc_voc(self.soc, temp_c)
            self.ib = 0.
            self.voc = self.voc_stat
            self.dv_dyn = 0.

        # EKF 1x1
        ddq_dt = self.ib
        if ddq_dt > 0. and not self.tweak_test:
            ddq_dt *= self.coul_eff
        ddq_dt -= dqdt*self.q_capacity*T_rate
        self.Q = self.scaler_q.calculate(ddq_dt)  # TODO this doesn't work right
        self.R = self.scaler_r.calculate(ddq_dt)  # TODO this doesn't work right
        self.Q = EKF_Q_SD_NORM**2  # override
        self.R = EKF_R_SD_NORM**2  # override
        self.predict_ekf(u=ddq_dt)  # u = d(q)/dt
        self.update_ekf(z=self.voc_stat, x_min=0., x_max=1.)  # z = voc, voc_filtered = hx
        self.soc_ekf = self.x_ekf  # x = Vsoc (0-1 ideal capacitor voltage) proxy for soc
        self.q_ekf = self.soc_ekf * self.q_capacity
        self.y_filt = self.y_filt_lag.calculate(in_=self.y_ekf, dt=min(dt, EKF_T_RESET), reset=False)
        self.y_filt2 = self.y_filt_2Ord.calculate(in_=self.y_ekf, dt=min(dt, TMAX_FILT), reset=False)

        # EKF convergence
        conv = abs(self.y_filt) < EKF_CONV
        self.EKF_converged.calculate(conv, EKF_T_CONV, EKF_T_RESET, min(dt, EKF_T_RESET), False)

        # Charge time
        if self.ib > 0.1:
            self.tcharge_ekf = min(Battery.RATED_BATT_CAP/self.ib * (1. - self.soc_ekf), 24.)
        elif self.ib < -0.1:
            self.tcharge_ekf = max(Battery.RATED_BATT_CAP/self.ib * self.soc_ekf, -24.)
        elif self.ib >= 0.:
            self.tcharge_ekf = 24.*(1. - self.soc_ekf)
        else:
            self.tcharge_ekf = -24.*self.soc_ekf

        self.Ib = self.ib * rp.nP
        self.Vb = self.vb * rp.nS
        self.Voc = self.voc * rp.nS
        self.Voc_stat = self.voc_stat * rp.nS
        self.Vsat = self.vsat * rp.nS
        self.dV_dyn = self.dv_dyn * rp.nS
        self.Voc_ekf = self.hx * rp.nS

        return self.soc_ekf

    def calc_charge_time(self, q, q_capacity, charge_curr, soc):
        delta_q = q - q_capacity
        if charge_curr > TCHARGE_DISPLAY_DEADBAND:
            self.tcharge = min(-delta_q / charge_curr / 3600., 24.)
        elif charge_curr < -TCHARGE_DISPLAY_DEADBAND:
            self.tcharge = max((q_capacity + delta_q - self.q_min) / charge_curr / 3600., -24.)
        elif charge_curr >= 0.:
            self.tcharge = 24.
        else:
            self.tcharge = -24.

        amp_hrs_remaining = (q_capacity - self.q_min + delta_q) / 3600.
        if soc > self.soc_min:
            self.amp_hrs_remaining_ekf = amp_hrs_remaining * (self.soc_ekf - self.soc_min) /\
                (soc - self.soc_min)
            self.amp_hrs_remaining_wt = amp_hrs_remaining * (self.soc - self.soc_min) /\
                (soc - self.soc_min)
        elif soc < self.soc_min:
            self.amp_hrs_remaining_ekf = amp_hrs_remaining * (self.soc_ekf - self.soc_min) / \
                                         (soc - self.soc_min)
            self.amp_hrs_remaining_wt = amp_hrs_remaining * (self.soc - self.soc_min) / \
                                        (soc - self.soc_min)
        else:
            self.amp_hrs_remaining_ekf = 0.
            self.amp_hrs_remaining_wt = 0.
        return self.tcharge

    # def count_coulombs(self, dt=0., reset=False, temp_c=25., charge_curr=0., sat=True):
    #     raise NotImplementedError

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

    def converged_ekf(self):
        return self.EKF_converged.state()

    def ekf_predict(self):
        """Process model"""
        self.Fx = math.exp(-self.dt / self.tau_sd)
        self.Bu = (1. - self.Fx)*self.r_sd
        return self.Fx, self.Bu

    def ekf_update(self):
        # Measurement function hx(x), x = soc ideal capacitor
        x_lim = max(min(self.x_ekf, 1.), 0.)
        self.hx, self.dv_dsoc = self.calc_soc_voc(x_lim, temp_c=self.temp_c)
        # Jacobian of measurement function
        self.H = self.dv_dsoc
        return self.hx, self.H

    def init_soc_ekf(self, soc):
        self.soc_ekf = soc
        self.init_ekf(soc, 0.0)
        self.q_ekf = self.soc_ekf * self.q_capacity

    def regauge(self, temp_c):
        if self.converged_ekf() and abs(self.soc_ekf - self.soc) > DF2:
            print("Resetting Coulomb Counter Monitor from ", self.soc, " to EKF=", self.soc_ekf, "...")
            self.apply_soc(self.soc_ekf, temp_c)
            print("confirmed ", self.soc)

    def save(self, time, dt, soc_ref, voc_ref):
        self.saved.time.append(time)
        self.saved.dt.append(dt)
        self.saved.ib.append(self.ib)
        self.saved.ioc.append(self.ioc)
        self.saved.vb.append(self.vb)
        self.saved.vc.append(self.vc())
        self.saved.vd.append(self.vd())
        self.saved.dv_hys.append(self.dv_hys)
        self.saved.dv_dyn.append(self.dv_dyn)
        self.saved.voc.append(self.voc)
        self.saved.voc_stat.append(self.voc_stat)
        self.saved.vbc.append(self.vbc())
        self.saved.vcd.append(self.vcd())
        self.saved.icc.append(self.i_c_ct())
        self.saved.irc.append(self.i_r_ct())
        self.saved.icd.append(self.i_c_dif())
        self.saved.ird.append(self.i_r_dif())
        self.saved.vcd_dot.append(self.vcd_dot())
        self.saved.vbc_dot.append(self.vbc_dot())
        self.saved.soc.append(self.soc)
        self.saved.soc_ekf.append(self.soc_ekf)
        self.saved.Fx.append(self.Fx)
        self.saved.Bu.append(self.Bu)
        self.saved.P.append(self.P)
        self.saved.H.append(self.H)
        self.saved.S.append(self.S)
        self.saved.K.append(self.K)
        self.saved.hx.append(self.hx)
        self.saved.u_ekf.append(self.u_ekf)
        self.saved.x_ekf.append(self.x_ekf)
        self.saved.y_ekf.append(self.y_ekf)
        self.saved.y_filt.append(self.y_filt)
        self.saved.y_filt2.append(self.y_filt2)
        self.saved.z_ekf.append(self.z_ekf)
        self.saved.x_prior.append(self.x_prior)
        self.saved.P_prior.append(self.P_prior)
        self.saved.x_post.append(self.x_post)
        self.saved.P_post.append(self.P_post)
        if abs(soc_ref) < 1e-6:
            soc_ref = 1e-6
        self.e_soc_ekf = (self.soc_ekf - soc_ref) / soc_ref
        self.e_voc_ekf = (self.voc - voc_ref) / voc_ref
        self.saved.e_soc_ekf.append(self.e_soc_ekf)
        self.saved.e_voc_ekf.append(self.e_voc_ekf)
        self.saved.Ib.append(self.Ib)
        self.saved.Vb.append(self.Vb)
        self.saved.Tb.append(self.temp_c)
        self.saved.Voc.append(self.Voc)
        self.saved.Voc_stat.append(self.Voc_stat)
        self.saved.Vsat.append(self.Vsat)
        self.saved.dV_dyn.append(self.dV_dyn)
        self.saved.Voc_ekf.append(self.Voc_ekf)
        self.saved.sat.append(int(self.sat))
        self.saved.sel.append(self.sel)
        self.saved.mod_data.append(self.mod)
        self.saved.soc_m.append(self.soc_m)
        self.Randles.save(time)


class BatteryModel(Battery):
    """Extend basic class to run a model"""

    def __init__(self, bat_v_sat=13.8, q_cap_rated=Battery.RATED_BATT_CAP * 3600,
                 t_rated=RATED_TEMP, t_rlim=0.017, scale=1.,
                 r_sd=70., tau_sd=2.5e7, r0=0.003, tau_ct=0.2, r_ct=0.0016, tau_dif=83., r_dif=0.0077,
                 temp_c=RATED_TEMP, hys_scale=1., tweak_test=False, dv_hys=0., sres=1.):
        Battery.__init__(self, bat_v_sat, q_cap_rated, t_rated,
                         t_rlim, r_sd, tau_sd, r0, tau_ct, r_ct, tau_dif, r_dif, temp_c, tweak_test, sres=sres)
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
        self.hys = Hysteresis(scale=hys_scale, dv_hys=dv_hys)  # Battery hysteresis model - drift of voc
        self.tweak_test = tweak_test
        self.voc = 0.  # Charging voltage, V
        self.d_delta_q = 0.  # Charging rate, Coulombs/sec
        self.charge_curr = 0.  # Charge current, A
        self.saved_m = SavedM()  # for plots and prints
        self.ib_in = 0.  # Saved value of current input, A
        self.ib_fut = 0.  # Future value of limited current, A

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix + "BatteryModel:\n"
        s += "  sat_ib_max =      {:7.3f}  // Current cutback to be applied to modeled ib output, A\n".\
            format(self.sat_ib_max)
        s += "  ib_null    =      {:7.3f}  // Current cutback value for voc=vsat, A\n".\
            format(self.sat_ib_null)
        s += "  sat_cutback_gain = {:6.2f}  // Gain to retard ib when voc exceeds vsat, dimensionless\n".\
            format(self.sat_cutback_gain)
        s += "  model_cutback =         {:d}  // Indicate that modeled current being limited on" \
             " saturation cutback, T = cutback limited\n".format(self.model_cutback)
        s += "  model_saturated =       {:f}  // Indicator of maximal cutback, T = cutback saturated\n".\
            format(self.model_saturated)
        s += "  ib_sat =          {:7.3f}  // Threshold to declare saturation.  This regeneratively slows" \
             " down charging so if too\n".format(self.ib_sat)
        s += "  ib_in  =          {:7.3f}  // Saved value of current input, A\n".format(self.ib_in)
        s += "  ib     =          {:7.3f}  // Open circuit current into posts, A\n".format(self.ib)
        s += "  ib_fut =          {:7.3f}  // Future value of limited current, A\n".format(self.ib_fut)
        s += "  voc     =         {:7.3f}  // Open circuit voltage, V\n".format(self.voc)
        s += "  voc_stat=         {:7.3f}  // Static, table lookup value of voc before applying hysteresis, V\n".\
            format(self.voc_stat)
        s += "  mod     =               {:f}  // Modeling\n".format(self.mod)
        s += "  \n  "
        s += self.hys.__str__(prefix + 'BatteryModel:')
        s += "  \n  "
        s += Battery.__str__(self, prefix + 'BatteryModel:')
        return s

    def calculate(self, temp_c, soc, curr_in, dt, q_capacity, dc_dc_on, reset, rp=None, sat_init=None):
        # BatteryModel
        self.dt = dt
        self.temp_c = temp_c
        self.mod = rp.modeling
        self.ib_in = curr_in
        if reset:
            self.ib_fut = self.ib_in
        self.ib = self.ib_fut

        soc_lim = max(min(soc, 1.), 0.)

        # VOC - OCV model
        self.voc_stat, self.dv_dsoc = self.calc_soc_voc(soc, temp_c)
        # slightly beyond but don't windup
        self.voc_stat = min(self.voc_stat + (soc - soc_lim) * self.dv_dsoc, self.vsat * 1.2)

        self.bms_off = ((self.temp_c < low_t) or (self.voc < low_voc)) and not self.tweak_test
        if self.bms_off:
            curr_in = 0.

        # Dynamic emf
        # Hysteresis model
        self.hys.calculate_hys(curr_in, self.soc)
        self.dv_hys = self.hys.update(self.dt)
        self.voc = self.voc_stat + self.dv_hys
        self.ioc = self.hys.ioc
        self.voc = self.voc
        # Randles dynamic model for model, reverse version to generate sensor inputs {ib, voc} --> {vb}, ioc=ib
        u = np.array([self.ib, self.voc]).T  # past value self.ib
        self.Randles.calc_x_dot(u)
        if dt < self.t_max:
            self.Randles.update(dt)
            self.vb = self.Randles.y
        else:  # aliased, unstable if update Randles
            self.vb = self.voc + self.r_ss * self.ib

        self.dv_dyn = self.vb - self.voc
        if self.bms_off and dc_dc_on:
            self.vb = 13.5
            self.dv_dyn = self.voc_stat
            self.voc = self.voc_stat

        # Saturation logic, both full and empty
        self.vsat = self.nom_vsat + (temp_c - 25.) * self.dvoc_dt
        self.sat_ib_max = self.sat_ib_null + (1 - self.soc) * self.sat_cutback_gain * rp.cutback_gain_scalar
        # if self.tweak_test:
        if self.tweak_test or (not rp.modeling):
            self.sat_ib_max = self.ib_in
        self.ib_fut = min(self.ib_in, self.sat_ib_max)  # the feedback of self.ib
        if (self.q <= -0.2*self.q_cap_rated_scaled) & (self.ib_in < 0.):  # empty
            self.ib_fut = 0.  # empty
        self.model_cutback = (self.voc_stat > self.vsat) & (self.ib_fut == self.sat_ib_max)
        self.model_saturated = (self.temp_c > low_t) and (self.model_cutback & (self.ib_fut < self.ib_sat))
        if reset and sat_init is not None:
            self.model_saturated = sat_init
            self.sat = sat_init
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
        a = np.array([[-1 / self.tau_ct, 0],
                      [0, -1 / self.tau_dif]])
        b = np.array([[1 / self.c_ct,   0],
                      [1 / self.c_dif,  0]])
        c = np.array([1., 1])
        d = np.array([self.r0, 1])
        return a, b, c, d

    def count_coulombs(self, dt, reset, temp_c, charge_curr, sat, soc_m_init=None, mon_delta_q=None, mon_sat=None):
        # BatteryModel
        """Coulomb counter based on true=actual capacity
        Internal resistance of battery is a loss
        Inputs:
            dt              Integration step, s
            temp_c          Battery temperature, deg C
            charge_curr     Charge, A
            sat             Indicator that battery is saturated (VOC>threshold(temp)), T/F
            t_last          Past value of battery temperature used for rate limit memory, deg C
            coul_eff_       Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs
        Outputs:
            soc     State of charge, fraction (0-1.5)
        """
        self.reset = reset
        self.charge_curr = charge_curr
        self.d_delta_q = self.charge_curr * dt
        if self.charge_curr > 0. and not self.tweak_test:
            self.d_delta_q *= self.coul_eff

        # Rate limit temperature
        self.temp_lim = max(min(temp_c, self.t_last + self.t_rlim*dt), self.t_last - self.t_rlim*dt)
        if self.reset:
            self.temp_lim = temp_c
            self.t_last = temp_c
            if soc_m_init and not self.mod:
                self.delta_q = self.calculate_capacity(temp_c) * (soc_m_init - 1.)

        # Saturation.   Goal is to set q_capacity and hold it so remembers last saturation status
        # detection
        if not self.mod and mon_sat:
            self.delta_q = mon_delta_q
        if self.model_saturated:
            if reset:
                self.delta_q = 0.  # Model is truth.   Saturate it then restart it to reset charge
        self.resetting = False  # one pass flag.  Saturation debounce should reset next pass

        # Integration can go to -20%
        self.q_capacity = self.calculate_capacity(self.temp_lim)
        self.delta_q += self.d_delta_q - dqdt*self.q_capacity*(self.temp_lim-self.t_last)
        self.delta_q = max(min(self.delta_q, 0.), -self.q_capacity*1.2)
        self.q = self.q_capacity + self.delta_q

        # Normalize
        self.soc = self.q / self.q_capacity
        self.soc_min = self.lut_soc_min.interp(self.temp_lim)
        self.q_min = self.soc_min * self.q_capacity

        # Save and return
        self.t_last = self.temp_lim
        return self.soc

    def save(self, time, dt):
        self.saved.time.append(time)
        self.saved.dt.append(dt)
        self.saved.ib.append(self.ib)
        self.saved.ioc.append(self.ioc)
        self.saved.vb.append(self.vb)
        self.saved.vc.append(self.vc())
        self.saved.vd.append(self.vd())
        self.saved.dv_hys.append(self.dv_hys)
        self.saved.dv_dyn.append(self.dv_dyn)
        self.saved.voc.append(self.voc)
        self.saved.voc_stat.append(self.voc_stat)
        self.saved.vbc.append(self.vbc())
        self.saved.vcd.append(self.vcd())
        self.saved.icc.append(self.i_c_ct())
        self.saved.irc.append(self.i_r_ct())
        self.saved.icd.append(self.i_c_dif())
        self.saved.ird.append(self.i_r_dif())
        self.saved.vcd_dot.append(self.vcd_dot())
        self.saved.vbc_dot.append(self.vbc_dot())
        self.saved.soc.append(self.soc)
        self.saved.d_delta_q.append(self.d_delta_q)
        self.saved.Tb.append(self.temp_c)
        self.saved.t_last.append(self.t_last)
        self.saved.Vsat.append(self.vsat)
        self.saved.charge_curr.append(self.charge_curr)
        self.saved.sat.append(int(self.model_saturated))
        self.saved.delta_q.append(self.delta_q)
        self.saved.q.append(self.q)
        self.saved.q_capacity.append(self.q_capacity)

    def save_m(self, time):
        self.saved_m.time.append(time)
        self.saved_m.Tb_m.append(self.temp_c)
        self.saved_m.Tbl_m.append(self.temp_lim)
        self.saved_m.vsat_m.append(self.vsat)
        self.saved_m.voc_m.append(self.voc)
        self.saved_m.voc_stat_m.append(self.voc_stat)
        self.saved_m.dv_dyn_m.append(self.dv_dyn)
        self.saved_m.vb_m.append(self.vb)
        self.saved_m.ib_m.append(self.ib)
        self.saved_m.ib_in_m.append(self.ib_in)
        self.saved_m.ib_fut_m.append(self.ib_fut)
        self.saved_m.sat_m.append(int(self.sat))
        self.saved_m.dq_m.append(self.delta_q)
        self.saved_m.soc_m.append(self.soc)
        self.saved_m.reset_m.append(self.reset)


# Other functions
def is_sat(temp_c, voc, soc):
    vsat = sat_voc(temp_c)
    return temp_c > low_t and (voc >= vsat or soc >= mxeps_bb)


def calculate_capacity(temp_c, t_sat, q_sat):
    return q_sat * (1-dqdt*(temp_c - t_sat))


def calc_vsat(temp_c):
    return sat_voc(temp_c)


def sat_voc(temp_c):
    return batt_vsat + (temp_c-25.)*BATT_DVOC_DT


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.dt = []
        self.ib = []
        self.ioc = []
        self.vb = []
        self.vc = []
        self.vd = []
        self.voc = []
        self.voc_stat = []
        self.dv_hys = []
        self.dv_dyn = []
        self.vbc = []
        self.vcd = []
        self.icc = []
        self.irc = []
        self.icd = []
        self.ird = []
        self.vcd_dot = []
        self.vbc_dot = []
        self.soc = []
        self.soc_ekf = []
        self.voc = []
        self.Fx = []
        self.Bu = []
        self.P = []
        self.H = []
        self.S = []
        self.K = []
        self.hx = []
        self.u_ekf = []
        self.x_ekf = []
        self.y_ekf = []
        self.y_filt = []
        self.y_filt2 = []
        self.z_ekf = []
        self.x_prior = []
        self.P_prior = []
        self.x_post = []
        self.P_post = []
        self.e_soc_ekf = []
        self.e_voc_ekf = []
        self.Ib = []  # Bank current, A
        self.Vb = []  # Bank voltage, V
        self.sat = []  # Indication that battery is saturated, T=saturated
        self.sel = []  # Current source selection, 0=amp, 1=no amp
        self.mod_data = []  # Configuration control code, 0=all hardware, 7=all simulated, +8 tweak test
        self.Tb = []  # Battery bank temperature, deg C
        self.Vsat = []  # Monitor Bank saturation threshold at temperature, deg C
        self.dV_dyn = []  # Monitor Bank current induced back emf, V
        self.Voc_stat = []  # Monitor Static bank open circuit voltage, V
        self.Voc = []  # Monitor Static bank open circuit voltage, V
        self.Voc_ekf = []  # Monitor bank solved static open circuit voltage, V
        self.y_ekf = []  # Monitor single battery solver error, V
        self.y_filt = []  # Filtered EKF y residual value, V
        self.y_filt2 = []  # Filtered EKF y residual value, V
        self.soc_m = []  # Simulated state of charge, fraction
        self.soc_ekf = []  # Solved state of charge, fraction
        # self.soc = []  # Coulomb Counter fraction of saturation charge (q_capacity_) available (0-1)
        self.d_delta_q = []  # Charging rate, Coulombs/sec
        self.charge_curr = []  # Charging current, A
        self.q = []  # Present charge available to use, except q_min_, C
        self.delta_q = []  # Charge change since saturated, C
        self.q_capacity = []  # Saturation charge at temperature, C
        self.t_last = []  # Past value of battery temperature used for rate limit memory, deg C


def overall(ms, ss, mrs, filename, fig_files=None, plot_title=None, n_fig=None, suffix=''):
    if fig_files is None:
        fig_files = []

    plt.figure()
    # plt.subplot_mosaic("AB;CD;CE")
    n_fig += 1
    plt.subplot(321)
    # plt.subplot_mosaic("A")
    plt.title(plot_title)
    plt.plot(ms.time, ms.ib, color='green', label='ib'+suffix)
    plt.plot(ss.time, ss.ioc, color='magenta', linestyle='--', label='ioc'+suffix)
    plt.plot(ms.time, ms.irc, color='red', linestyle='-.', label='i_r_ct'+suffix)
    plt.plot(ms.time, ms.icd, color='cyan', linestyle=':', label='i_c_dif'+suffix)
    plt.plot(ms.time, ms.ird, color='orange', linestyle=':', label='i_r_dif'+suffix)
    # plt.plot(ms.time, ms.ib, color='black', linestyle='--', label='Ioc'+suffix)
    plt.legend(loc=1)
    plt.subplot(323)
    # plt.subplot_mosaic("C")
    plt.plot(ms.time, ms.vb, color='green', label='vb'+suffix)
    plt.plot(ss.time, ss.vb, color='black', linestyle='--', label='vb_m'+suffix)
    plt.plot(ms.time, ms.vc, color='blue', linestyle='-.', label='vc'+suffix)
    plt.plot(ss.time, ss.vc, color='green', linestyle=':', label='vc_m'+suffix)
    plt.plot(ms.time, ms.vd, color='red', label='vd'+suffix)
    plt.plot(ss.time, ss.vd, color='orange', linestyle='--', label='vd_m'+suffix)
    plt.plot(ms.time, ms.voc_stat, color='orange', linestyle='-.', label='voc_stat'+suffix)
    plt.plot(ss.time, ss.voc_stat, color='cyan', linestyle=':', label='voc_stat_,'+suffix)
    plt.plot(ms.time, ms.voc, color='magenta', label='voc'+suffix)
    plt.plot(ss.time, ss.voc, color='black', linestyle='--', label='voc_m'+suffix)
    plt.legend(loc=1)
    plt.subplot(324)
    # plt.subplot_mosaic("D")
    plt.plot(ms.time, ms.vbc_dot, color='green', label='vbc_dot'+suffix)
    plt.plot(ms.time, ms.vcd_dot, color='blue', label='vcd_dot'+suffix)
    plt.legend(loc=1)
    plt.subplot(322)
    # plt.subplot_mosaic("B")
    plt.plot(ms.time, ms.soc, color='red', label='soc'+suffix)
    plt.plot(ss.time, ss.soc, color='black', linestyle='dotted', label='soc_m'+suffix)
    plt.legend(loc=1)
    plt.subplot(326)
    # plt.subplot_mosaic("E")
    plt.plot(ss.soc, ss.voc, color='black', linestyle='-.', label='SIM voc vs soc'+suffix)
    plt.plot(ms.soc, ms.voc, color='green', linestyle=':', label='MON voc vs soc'+suffix)
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.subplot(111)
    plt.title(plot_title)
    plt.plot(ms.time, ms.vb, color='green', label='vb'+suffix)
    plt.plot(ss.time, ss.vb, color='black', linestyle='--', label='vb_m'+suffix)
    plt.plot(ms.time, ms.vc, color='blue', linestyle='-.', label='vc'+suffix)
    plt.plot(ss.time, ss.vc, color='green', linestyle=':', label='vc_m'+suffix)
    plt.plot(ms.time, ms.vd, color='red', label='vd'+suffix)
    plt.plot(ss.time, ss.vd, color='orange', linestyle='--', label='vd_m'+suffix)
    plt.plot(ms.time, ms.voc_stat, color='orange', linestyle='-.', label='voc_stat'+suffix)
    plt.plot(ss.time, ss.voc_stat, color='cyan', linestyle=':', label='voc_stat'+suffix)
    plt.plot(ms.time, ms.voc, color='magenta', label='voc'+suffix)
    plt.plot(ss.time, ss.voc, color='black', linestyle='--', label='voc_m'+suffix)
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.subplot(321)
    plt.title(plot_title+' **SIM')
    # plt.plot(ss.time, ref, color='black', label='curr dmd, A'+suffix)
    plt.plot(ss.time, ss.ib, color='green', label='ib'+suffix)
    plt.plot(ss.time, ss.irc, color='red',  linestyle='--', label='i_r_ct'+suffix)
    plt.plot(ss.time, ss.icd, color='cyan',  linestyle='-.', label='i_c_dif'+suffix)
    plt.plot(ss.time, ss.ird, color='orange', linestyle=':', label='i_r_dif'+suffix)
    # plt.plot(ss.time, ss.ib, color='black', linestyle='--', label='Ioc'+suffix)
    plt.legend(loc=1)
    plt.subplot(323)
    plt.plot(ss.time, ss.vb, color='green', label='vb'+suffix)
    plt.plot(ss.time, ss.vc, color='blue', linestyle='--', label='vc'+suffix)
    plt.plot(ss.time, ss.vd, color='red', linestyle='-.', label='vd'+suffix)
    plt.plot(ss.time, ss.voc, color='orange', label='voc'+suffix)
    plt.plot(ss.time, ss.voc_stat, color='magenta', linestyle=':', label='voc stat'+suffix)
    plt.legend(loc=1)
    plt.subplot(325)
    plt.plot(ss.time, ss.vbc_dot, color='green', label='vbc_dot'+suffix)
    plt.plot(ss.time, ss.vcd_dot, color='blue', label='vcd_dot'+suffix)
    plt.legend(loc=1)
    plt.subplot(322)
    plt.plot(ss.time, ss.soc, color='red', label='soc'+suffix)
    plt.legend(loc=1)
    plt.subplot(326)
    plt.plot(ss.soc, ss.voc, color='black', label='voc vs soc'+suffix)
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 3
    n_fig += 1
    plt.subplot(321)
    plt.title(plot_title+' MON vs SIM')
    plt.plot(ms.time, ms.ib, color='green', label='ib'+suffix)
    plt.plot(ss.time, ss.ib, color='black', linestyle='--', label='ib_m'+suffix)
    plt.legend(loc=1)
    plt.subplot(322)
    plt.plot(ms.time, ms.vb, color='green', label='vb'+suffix)
    plt.plot(ss.time, ss.vb, color='black', linestyle='--', label='vb_m'+suffix)
    plt.plot(ms.time, ms.voc, color='cyan', label='voc'+suffix)
    plt.plot(ss.time, ss.voc, color='red', linestyle='--', label='voc_m'+suffix)
    plt.legend(loc=1)
    plt.subplot(323)
    plt.plot(ms.time, ms.vb, color='green', label='vb'+suffix)
    plt.plot(ss.time, ss.vb, color='orange', linestyle='--', label='vb_m'+suffix)
    plt.plot(ms.time, ms.voc, color='cyan', linestyle='-.', label='voc'+suffix)
    plt.plot(ss.time, ss.voc, color='red', linestyle=':', label='voc_m'+suffix)
    plt.plot(ms.time, ms.voc_stat, color='magenta', linestyle=':', label='voc_stat'+suffix)
    plt.plot(ss.time, ss.voc_stat, color='black', linestyle='--', label='voc_stat_m'+suffix)
    plt.legend(loc=1)
    plt.subplot(324)
    plt.plot(ss.time, ss.dv_dyn, color='green', label='dv_dyn'+suffix)
    plt.plot(ms.time, ms.dv_dyn, color='red', linestyle='--', label='dv_dyn_m'+suffix)
    plt.legend(loc=1)
    plt.subplot(325)
    plt.plot(ms.time, ms.dv_hys, color='green', label='dv_hys'+suffix)
    plt.plot(ss.time, ss.dv_hys, color='black', linestyle='--', label='dv_hys_m'+suffix)
    plt.legend(loc=1)
    plt.subplot(326)
    plt.plot(ms.time, ms.vb, color='green', label='vb'+suffix)
    plt.plot(ss.time, ss.vb, color='black', linestyle='--', label='vb_m'+suffix)
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.subplot(331)
    plt.title(plot_title+' **EKF')
    plt.plot(ms.time, ms.x_ekf, color='red', label='x ekf'+suffix)
    plt.legend(loc=4)
    plt.subplot(332)
    plt.plot(ms.time, ms.hx, color='cyan', label='hx ekf'+suffix)
    plt.plot(ms.time, ms.z_ekf, color='black', linestyle='--', label='z ekf'+suffix)
    plt.legend(loc=4)
    plt.subplot(333)
    plt.plot(ms.time, ms.y_ekf, color='green', label='y ekf'+suffix)
    plt.plot(ms.time, ms.y_filt, color='black', linestyle='--', label='y filt'+suffix)
    plt.plot(ms.time, ms.y_filt2, color='cyan', linestyle='-.', label='y filt2'+suffix)
    plt.legend(loc=4)
    plt.subplot(334)
    plt.plot(ms.time, ms.H, color='magenta', label='H ekf'+suffix)
    plt.ylim(0, 150)
    plt.legend(loc=3)
    plt.subplot(335)
    plt.plot(ms.time, ms.P, color='orange', label='P ekf'+suffix)
    plt.legend(loc=3)
    plt.subplot(336)
    plt.plot(ms.time, ms.Fx, color='red', label='Fx ekf'+suffix)
    plt.legend(loc=2)
    plt.subplot(337)
    plt.plot(ms.time, ms.Bu, color='blue', label='Bu ekf'+suffix)
    plt.legend(loc=2)
    plt.subplot(338)
    plt.plot(ms.time, ms.K, color='red', label='K ekf'+suffix)
    plt.legend(loc=4)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.title(plot_title)
    plt.plot(ms.time, ms.e_voc_ekf, color='blue', linestyle='-.', label='e_voc'+suffix)
    plt.plot(ms.time, ms.e_soc_ekf, color='red', linestyle='dotted', label='e_soc_ekf'+suffix)
    plt.ylim(-0.01, 0.01)
    plt.legend(loc=2)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.title(plot_title)
    plt.plot(ms.time, ms.voc, color='red', label='voc'+suffix)
    plt.plot(ms.time, ms.Voc_ekf, color='blue', linestyle='-.', label='Voc_ekf'+suffix)
    plt.plot(ss.time, ss.voc, color='green', linestyle=':', label='voc_m'+suffix)
    plt.legend(loc=4)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.title(plot_title)
    plt.plot(ms.time, ms.soc_ekf, color='blue', label='soc_ekf'+suffix)
    plt.plot(ss.time, ss.soc, color='green', linestyle='-.', label='soc_m'+suffix)
    plt.plot(ms.time, ms.soc, color='red', linestyle=':', label='soc'+suffix)
    plt.legend(loc=4)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.title(plot_title)
    plt.plot(ms.time, ms.e_voc_ekf, color='blue', linestyle='-.', label='e_voc'+suffix)
    plt.plot(ms.time, ms.e_soc_ekf, color='red', linestyle='dotted', label='e_soc_ekf'+suffix)
    plt.legend(loc=2)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title)
    plt.plot(ss.time, ss.soc, color='red', label='soc'+suffix)
    plt.legend(loc=1)
    plt.subplot(223)
    plt.plot(ss.time, ss.ib, color='blue', label='ib, A'+suffix)
    plt.plot(ss.time, ss.ioc, color='green', label='ioc hys indicator, A'+suffix)
    plt.legend(loc=1)
    plt.subplot(224)
    plt.plot(ss.time, ss.dv_hys, color='red', label='dv_hys, V'+suffix)
    plt.legend(loc=2)
    fig_file_name = filename + "_" + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.subplot(111)
    plt.title(plot_title)
    plt.plot(ss.soc, ss.voc, color='black', linestyle='dotted', label='SIM voc vs soc'+suffix)
    plt.legend(loc=2)
    fig_file_name = filename + "_" + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title)
    plt.plot(mrs.time[1:], mrs.u[1:, 1], color='red', label='Mon Randles u[2]=vb'+suffix)
    plt.plot(mrs.time[1:], mrs.y[1:], color='blue', label='Mon Randles y=voc'+suffix)
    plt.legend(loc=2)
    plt.subplot(222)
    plt.plot(mrs.time[1:], mrs.x[1:, 0], color='blue', label='Mon Randles x[1]'+suffix)
    plt.plot(mrs.time[1:], mrs.x[1:, 1], color='red', label='Mon Randles x[2]'+suffix)
    plt.legend(loc=2)
    plt.subplot(223)
    plt.plot(mrs.time[1:], mrs.x_dot[1:, 0], color='blue', label='Mon Randles x_dot[1]'+suffix)
    plt.plot(mrs.time[1:], mrs.x_dot[1:, 1], color='red', label='Mon Randles x_dot[2]'+suffix)
    plt.legend(loc=2)
    plt.subplot(224)
    plt.plot(mrs.time[1:], mrs.u[1:, 0], color='blue', label='Mon Randles u[1]=Ib'+suffix)
    plt.legend(loc=2)
    fig_file_name = filename + "_" + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files


class SavedM:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.unit = []  # text title
        self.c_time = []  # Control time, s
        self.Tb_m = []
        self.Tbl_m = []
        self.vsat_m = []
        self.voc_m = []
        self.voc_stat_m = []
        self.dv_dyn_m = []
        self.vb_m = []
        self.ib_m = []
        self.ib_in_m = []
        self.ib_fut_m = []
        self.sat_m = []
        self.ddq_m = []
        self.dq_m = []
        self.q_m = []
        self.qcap_m = []
        self.soc_m = []
        self.reset_m = []

    def __str__(self):
        s = "unit_m,c_time,Tb_m,Tbl_m,vsat_m,voc_stat_m,dv_dyn_m,vb_m,ib_m,sat_m,ddq_m,dq_m,q_m,qcap_m,soc_m,reset_m,\n"
        for i in range(len(self.time)):
            s += 'sim,'
            s += "{:13.3f},".format(self.time[i])
            s += "{:5.2f},".format(self.Tb_m[i])
            s += "{:5.2f},".format(self.Tbl_m[i])
            s += "{:8.3f},".format(self.vsat_m[i])
            s += "{:5.2f},".format(self.voc_stat_m[i])
            s += "{:5.2f},".format(self.dv_dyn_m[i])
            s += "{:5.2f},".format(self.vb_m[i])
            s += "{:8.3f},".format(self.ib_m[i])
            s += "{:8.3f},".format(self.ib_in_m[i])
            s += "{:8.3f},".format(self.ib_fut_m[i])
            s += "{:1.0f},".format(self.sat_m[i])
            s += "{:5.3f},".format(self.ddq_m[i])
            s += "{:5.3f},".format(self.dq_m[i])
            s += "{:5.3f},".format(self.qcap_m[i])
            s += "{:7.3f},".format(self.soc_m[i])
            s += "{:d},".format(self.reset_m[i])
            s += "\n"
        return s
