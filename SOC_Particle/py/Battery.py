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
from EKF1x1 import EKF1x1
from Coulombs import Coulombs
from Hysteresis import Hysteresis
import matplotlib.pyplot as plt
from TFDelay import TFDelay
from myFilters import LagTustin, LagExp, General2Pole, RateLimit, SlidingDeadband
from Scale import Scale

plt.rcParams.update({'figure.max_open_warning': 0})


class Retained:

    def __init__(self):
        self.cutback_gain_scalar = 1.
        self.delta_q = 0.
        self.t_last = 25.
        self.delta_q_model = 0.
        self.t_last_model = 25.
        self.modeling = 7  # assumed for this 'model'; over-ridden later

    def tweak_test(self):
        return 0b1000 & int(self.modeling)


class Battery(Coulombs):
    # Battery constants
    UNIT_CAP_RATED = 100.
    NOM_SYS_VOLT = 12.  # Nominal system output, V, at which the reported amps are used (12)
    mxeps_bb = 1.05  # Numerical maximum of coefficient model with scaled soc
    TCHARGE_DISPLAY_DEADBAND = 0.1  # Inside this +/- deadband, charge time is displayed '---', A
    DF1 = 0.02  # Weighted selection lower transition drift, fraction
    DF2 = 0.70  # Threshold to reset Coulomb Counter if different from ekf, fraction (0.05)
    EKF_CONV = 2e-3  # EKF tracking error indicating convergence, V (1e-3)
    EKF_T_CONV = 30.  # EKF set convergence test time, sec (30.)
    EKF_T_RESET = (EKF_T_CONV / 2.)  # EKF reset test time, sec ('up 1, down 2')
    EKF_NOM_DT = 0.1  # EKF nominal update time, s (initialization; actual value varies)
    TAU_Y_FILT = 5.  # EKF y-filter time constant, sec (5.)
    MIN_Y_FILT = -0.5  # EKF y-filter minimum, V (-0.5)
    MAX_Y_FILT = 0.5  # EKF y-filter maximum, V (0.5)
    WN_Y_FILT = 0.1  # EKF y-filter-2 natural frequency, r/s (0.1)
    ZETA_Y_FILT = 0.9  # EKF y-filter-2 damping factor (0.9)
    TMAX_FILT = 3.  # Maximum y-filter-2 sample time, s (3.)
    EKF_Q_SD_NORM = 0.0015  # Standard deviation of normal EKF process uncertainty, V (0.0015)
    EKF_R_SD_NORM = 0.5  # Standard deviation of normal EKF state uncertainty, fraction (0-1) (0.5)
    IMAX_NUM = 100000.  # Overflow protection since ib past value used
    HYS_SOC_MIN_MARG = 0.15  # Add to soc_min to set thr for detecting low endpoint condition for reset of hysteresis
    HYS_SOC_MAX = 0.99  # Detect high endpoint condition for reset of hysteresis
    HYS_E_WRAP_THR = 0.1  # Detect e_wrap going the other way; need to reset dv_hys at endpoints
    HYS_IB_THR = 1.  # Ignore reset if opposite situation exists
    IB_MIN_UP = 0.2  # Min up charge current for come alive, BMS logic, and fault
    cp_eframe_mult = 20  # Run EKF 20 times slower than Coulomb Counter
    READ_DELAY = 100  # nominal read time, ms
    EKF_EFRAME_MULT = 20  # Multi-frame rate consistent with READ_DELAY (20 for READ_DELAY=100)
    VB_DC_DC = 13.5  # Estimated dc-dc charger, V
    HDB_VBATT = 0.05  # Half deadband to filter vb, V (0.05)
    WRAP_ERR_FILT = 4.  # Wrap error filter time constant, s (4)
    MAX_WRAP_ERR_FILT = 10.  # Anti-windup wrap error filter, V (10)
    F_MAX_T_WRAP = 2.8  # Maximum update time of Wrap filter for stability at WRAP_ERR_FILT, s (2.8)
    D_SOC_S = 0.  # Bias on soc to voc-soc lookup to simulate error in estimation, esp cold battery near 0 C
    # """Nominal battery bank capacity, Ah(100).Accounts for internal losses.This is
    #                         what gets delivered, e.g. Wshunt / NOM_SYS_VOLT.  Also varies 0.2 - 0.4 C currents
    #                         or 20 - 40 A for a 100 Ah battery"""

    # Battery model:  Randles' dynamics, SOC-VOC model

    """Nominal battery bank capacity, Ah(100).Accounts for internal losses.This is
                            what gets delivered, e.g.Wshunt / NOM_SYS_VOLT.  Also varies 0.2 - 0.4 C currents
                            or 20 - 40 A for a 100 Ah battery"""

    def __init__(self, q_cap_rated=UNIT_CAP_RATED*3600, temp_rlim=0.017, t_rated=25., temp_c=25., tweak_test=False,
                 sres0=1., sresct=1., stauct=1., scale_r_ss=1., s_hys=1., dvoc=0., mod_code=0, s_coul_eff=1.,
                 scale_cap=1.):
        """ Default values from Taborelli & Onori, 2013, State of Charge Estimation Using Extended Kalman Filters for
        Battery Management System.   Battery equations from LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang
        etal.pdf.'  SOC-OCV curve fit './Battery State/BattleBorn Rev1.xls:Model Fit' using solver with min slope
        constraint >=0.02 V/soc.  m and n using Zhang values.   Had to scale soc because  actual capacity > NOM_BAT_CAP
        so equation error when soc<=0 to match data.    See Battery.h
        """
        # Parents
        Coulombs.__init__(self, q_cap_rated,  q_cap_rated, t_rated, temp_rlim, tweak_test, mod_code=mod_code, dvoc=dvoc)

        # Defaults
        self.chem = mod_code
        self.dvoc = dvoc
        self.nz = None
        self.q = 0  # Charge, C
        self.voc = Battery.NOM_SYS_VOLT  # Model open circuit voltage, V
        self.voc_stat = self.voc  # Static model open circuit voltage from charge process, V
        self.dv_dyn = 0.  # Model current induced back emf, V
        self.vb = Battery.NOM_SYS_VOLT  # Battery voltage at post, V
        self.ib = 0.  # Current into battery post, A
        self.ib_in = 0.  # Current into calculate, A
        self.ib_charge = 0.  # Current into count_coulombs, A
        self.ioc = 0  # Current into battery process accounting for hysteresis, A
        self.dv_dsoc = 0.  # Slope of soc-voc curve, V/%
        self.tcharge = 0.  # Charging time to 100%, hr
        self.sr = 1  # Resistance scalar
        self.vsat = self.chemistry.nom_vsat
        # range 0 - 50 C, V/deg C
        self.dt = 0  # Update time, s
        self.chemistry.r_0 *= sres0
        self.chemistry.tau_ct *= stauct
        self.chemistry.r_ct *= sresct
        self.chemistry.r_ss *= scale_r_ss
        self.chemistry.coul_eff *= s_coul_eff
        self.ChargeTransfer = LagExp(dt=Battery.EKF_NOM_DT, max_=Battery.UNIT_CAP_RATED*scale_cap,
                                     min_=-Battery.UNIT_CAP_RATED*scale_cap, tau=self.chemistry.tau_ct)
        self.temp_c = temp_c
        self.saved = Saved()  # for plots and prints
        self.dv_hys = 0.  # Placeholder so BatterySim can be plotted
        self.tau_hys = 0.  # Placeholder so BatterySim can be plotted
        self.dv_dyn = 0.  # Placeholder so BatterySim can be plotted
        self.bms_off = False
        self.mod = 7
        self.sel = 0
        self.tweak_test = tweak_test
        self.s_hys = s_hys
        self.ib_lag = 0.
        self.IbLag = LagExp(1., 1., -100., 100.)  # Lag to be run on sat to produce ib_lag.  T and tau set at run time
        self.voc_soc_new = 0.

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix + "Battery:\n"
        s += "  chem    = {:7.3f}  // Chemistry: 0=Battleborn, 1=CHINS\n".format(self.chem)
        s += "  temp_c  = {:7.3f}  // Battery temperature, deg C\n".format(self.temp_c)
        s += "  dvoc_dt = {:9.6f}  // Change of VOC with operating temperature in range 0 - 50 C V/deg C\n"\
            .format(self.chemistry.dvoc_dt)
        s += "  r_0     = {:9.6f}  // Charge Transfer R0, ohms\n".format(self.chemistry.r_0)
        s += "  r_ct    = {:9.6f}  // Charge Transfer resistance, ohms\n".format(self.chemistry.r_ct)
        s += "  tau_ct = {:9.6f}  // Charge Transfer time constant, s (=1/Rdif/Cdif)\n".format(self.chemistry.tau_ct)
        s += "  r_ss    = {:9.6f}  // Steady state equivalent battery resistance, for solver, Ohms\n"\
            .format(self.chemistry.r_ss)
        s += "  r_sd    = {:9.6f}  // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms\n"\
            .format(self.chemistry.r_sd)
        s += "  tau_sd  = {:9.1f}  // Equivalent model for EKF reference.	Parasitic discharge time constant, sec\n"\
            .format(self.chemistry.tau_sd)
        s += "  bms_off  = {:7.1f}      // BMS off\n".format(self.bms_off)
        s += "  dv_dsoc = {:9.6f}  // Derivative scaled, V/fraction\n".format(self.dv_dsoc)
        s += "  ib =      {:7.3f}  // Battery terminal current, A\n".format(self.ib)
        s += "  vb =      {:7.3f}  // Battery terminal voltage, V\n".format(self.vb)
        s += "  voc      ={:7.3f}  // Static model open circuit voltage, V\n".format(self.voc)
        s += "  voc_stat ={:7.3f}  // Static model open circuit voltage from table (reference), V\n"\
            .format(self.voc_stat)
        s += "  vsat =    {:7.3f}  // Saturation threshold at temperature, V\n".format(self.vsat)
        s += "  dv_dyn =  {:7.3f}  // Current-induced back emf, V\n".format(self.dv_dyn)
        s += "  q =       {:7.3f}  // Present charge available to use, except q_min_, C\n".format(self.q)
        s += "  sr =      {:7.3f}  // Resistance scalar\n".format(self.sr)
        s += "  dvoc_ =   {:7.3f}  // Delta voltage, V\n".format(self.dvoc)
        s += "  dt_ =     {:7.3f}  // Update time, s\n".format(self.dt)
        s += "  dv_hys  = {:7.3f}  // Hysteresis delta v, V\n".format(self.dv_hys)
        s += "  tau_hys = {:7.3f}  // Hysteresis time const, s\n".format(self.tau_hys)
        s += "  tweak_test={:d}     // Driving signal injection completely using software inj_soft_bias\n"\
            .format(self.tweak_test)
        s += "\n  "
        s += Coulombs.__str__(self, prefix + 'Battery:')
        return s

    def assign_temp_c(self, temp_c):
        self.temp_c = temp_c

    def assign_soc(self, soc, voc):
        self.soc = soc
        self.voc = voc
        self.voc_stat = voc
        self.vsat = self.chemistry.nom_vsat + (self.temp_c - self.chemistry.rated_temp) * self.chemistry.dvoc_dt
        self.sat = self.voc >= self.vsat

    def calc_h_jacobian(self, soc_lim, temp_c):
        if soc_lim > 0.5:
            dv_dsoc = (self.chemistry.lut_voc_soc.interp(soc_lim, temp_c) -
                       self.chemistry.lut_voc_soc.interp(soc_lim-0.01, temp_c)) / 0.01
        else:
            dv_dsoc = (self.chemistry.lut_voc_soc.interp(soc_lim+0.01, temp_c) -
                       self.chemistry.lut_voc_soc.interp(soc_lim, temp_c)) / 0.01
        return dv_dsoc

    def calc_soc_voc(self, soc, temp_c):
        """SOC-OCV curve fit method per Zhang, etal """
        dv_dsoc = self.calc_h_jacobian(soc, temp_c)
        voc = self.chemistry.lut_voc_soc.interp(soc, temp_c) + self.dvoc
        # print("soc=", soc, "temp_c=", temp_c, "dvoc=", self.dvoc, "voc=", voc)
        return voc, dv_dsoc

    def calculate(self, chem, temp_c, soc, curr_in, dt, q_capacity, dc_dc_on, reset, update_time_in,  # BatterySim
                  rp=None, sat_init=None, bms_off_init=None):
        # Battery
        raise NotImplementedError

    def look_hys(self, dv, soc):
        raise NotImplementedError


class BatteryMonitor(Battery, EKF1x1):
    """Extend Battery class to make a monitor"""

    def __init__(self, q_cap_rated=Battery.UNIT_CAP_RATED*3600, t_rated=25., temp_rlim=0.017, scale=1.,
                 temp_c=25., tweak_test=False, sres0=1., sresct=1., stauct=1.,
                 scaler_q=None, scaler_r=None, scale_r_ss=1., s_hys=1., dvoc=0., eframe_mult=Battery.cp_eframe_mult,
                 mod_code=0, s_coul_eff=1.):
        q_cap_rated_scaled = q_cap_rated * scale
        Battery.__init__(self, q_cap_rated=q_cap_rated_scaled, t_rated=t_rated, temp_rlim=temp_rlim, temp_c=temp_c,
                         tweak_test=tweak_test, sres0=sres0, sresct=sresct, stauct=stauct, scale_r_ss=scale_r_ss,
                         s_hys=s_hys, dvoc=dvoc, mod_code=mod_code, s_coul_eff=s_coul_eff, scale_cap=scale)

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
            self.scaler_q = Scale(1, 4, Battery.EKF_Q_SD_NORM, Battery.EKF_Q_SD_NORM)
        else:
            self.scaler_q = scaler_q
        if scaler_r is None:
            self.scaler_r = Scale(1, 4, Battery.EKF_R_SD_NORM, Battery.EKF_R_SD_NORM)
        else:
            self.scaler_r = scaler_r
        self.Q = Battery.EKF_Q_SD_NORM * Battery.EKF_Q_SD_NORM  # EKF process uncertainty
        self.R = Battery.EKF_R_SD_NORM * Battery.EKF_R_SD_NORM  # EKF state uncertainty
        self.soc_s = 0.  # Model information
        self.EKF_converged = TFDelay(False, Battery.EKF_T_CONV, Battery.EKF_T_RESET, Battery.EKF_NOM_DT)
        self.y_filt_lag = LagTustin(0.1, Battery.TAU_Y_FILT, Battery.MIN_Y_FILT, Battery.MAX_Y_FILT)
        self.WrapErrFilt = LagTustin(0.1, Battery.WRAP_ERR_FILT, -Battery.MAX_WRAP_ERR_FILT, Battery.MAX_WRAP_ERR_FILT)
        self.y_filt = 0.
        self.y_filt_2Ord = General2Pole(0.1, Battery.WN_Y_FILT, Battery.ZETA_Y_FILT, Battery.MIN_Y_FILT,
                                        Battery.MAX_Y_FILT)
        self.y_filt2 = 0.
        self.ib = 0.
        self.vb = 0.
        self.vb_model_rev = 0.
        self.voc_stat = 0.
        self.voc_soc = 0.
        self.voc = 0.
        self.voc_filt = 0.
        self.vsat = 0.
        self.dv_dyn = 0.
        self.voc_ekf = 0.
        self.Temp_Rlim = RateLimit()
        self.eframe = 0
        self.eframe_mult = eframe_mult
        self.dt_eframe = self.dt*self.eframe_mult
        self.sdb_voc = SlidingDeadband(Battery.HDB_VBATT)
        self.e_wrap = 0.
        self.e_wrap_filt = 0.
        self.reset_past = True
        self.ib_past = 0.

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix
        s += Battery.__str__(self, prefix + 'BatteryMonitor:')
        s += "  amp_hrs_remaining_ekf_ =  {:7.3f}  // Discharge amp*time left if drain to q_ekf=0, A-h\n"\
            .format(self.amp_hrs_remaining_ekf)
        s += "  amp_hrs_remaining_wt_  =  {:7.3f}  // Discharge amp*time left if drain soc_wt_ to 0, A-h\n"\
            .format(self.amp_hrs_remaining_wt)
        s += "  q_ekf     {:7.3f}  // Filtered charge calculated by ekf, C\n".format(self.q_ekf)
        s += "  voc_soc  ={:7.3f}  // Static model open circuit voltage from table (reference), V\n"\
            .format(self.voc_soc)
        s += "  soc_ekf = {:7.3f}  // Solved state of charge, fraction\n".format(self.soc_ekf)
        s += "  tcharge = {:7.3f}  // Charging time to full, hr\n".format(self.tcharge)
        s += "  tcharge_ekf = {:7.3f}   // Charging time to full from ekf, hr\n".format(self.tcharge_ekf)
        s += "  mod     =               {:f}  // Modeling\n".format(self.mod)
        s += "\n  "
        s += EKF1x1.__str__(self, prefix + 'BatteryMonitor:')
        return s

    def assign_soc_s(self, soc_s):
        self.soc_s = soc_s

    # BatteryMonitor::calculate()
    def calculate(self, chem, temp_c, vb, ib, dt, reset, update_time_in, q_capacity=None, dc_dc_on=None,
                  rp=None, u_old=None, z_old=None, x_old=None, bms_off_init=None, nS=1):
        if self.chm != chem:
            self.chemistry.assign_all_mod(chem)
            self.chm = chem

        self.temp_c = temp_c
        self.vsat = calc_vsat(self.temp_c, self.chemistry.nom_vsat, self.chemistry.dvoc_dt)
        self.dt = dt
        self.ib_in = ib
        self.mod = rp.modeling
        self.Temp_Rlim.update(x=self.temp_c, reset=reset, dt=self.dt, max_=0.017, min_=-.017)
        temp_rate = self.Temp_Rlim.rate
        # Overflow protection since ib past value used
        self.ib = max(min(self.ib_in, Battery.IMAX_NUM), -Battery.IMAX_NUM)

        # Table lookup
        self.voc_soc, self.dv_dsoc = self.calc_soc_voc(self.soc, temp_c)

        # Battery management system model (uses past value bms_off and voc_stat)
        if not self.bms_off:
            voltage_low = self.voc_stat < self.chemistry.vb_down
        else:
            voltage_low = self.voc_stat < self.chemistry.vb_rising
        bms_charging = self.ib > Battery.IB_MIN_UP
        if reset and bms_off_init is not None:
            self.bms_off = bms_off_init
        else:
            self.bms_off = (self.temp_c <= self.chemistry.low_t) or (voltage_low and not rp.tweak_test())  # KISS
        self.ib_charge = self.ib / nS
        if self.bms_off and not bms_charging:
            self.ib_charge = 0.
        if self.bms_off and voltage_low:
            self.ib = 0.
        self.ib_lag = self.IbLag.calculate_tau(self.ib, reset, self.dt, self.chemistry.ib_lag_tau)
        if reset:
            self.ib_past = self.ib

        # Dynamic emf
        if rp.modeling:
            ib_dyn = self.ib_past
        else:
            ib_dyn = self.ib
        self.vb = vb
        self.voc = self.vb - (self.ChargeTransfer.calculate(ib_dyn, reset, dt)*self.chemistry.r_ct +
                              ib_dyn*self.chemistry.r_0)
        if self.bms_off and voltage_low:
            self.voc_stat = self.vb
            self.voc = self.vb
        self.dv_dyn = self.vb - self.voc

        # Hysteresis model
        self.dv_hys = 0.
        self.voc_stat = self.voc - self.dv_hys
        self.e_wrap = self.voc_soc - self.voc_stat
        self.ioc = self.ib / nS
        self.e_wrap_filt = self.WrapErrFilt.calculate(in_=self.e_wrap, dt=min(self.dt, Battery.F_MAX_T_WRAP),
                                                      reset=reset)

        # Reversionary model
        self.vb_model_rev = self.voc_soc + self.dv_dyn + self.dv_hys

        # EKF 1x1
        self.eframe_mult = max(int(float(Battery.READ_DELAY)*float(Battery.EKF_EFRAME_MULT)/1000./self.dt + 0.9999), 1)
        if self.eframe == 0:
            self.dt_eframe = self.dt * float(self.eframe_mult)
            ddq_dt = self.ib_charge
            if ddq_dt > 0. and not self.tweak_test:
                ddq_dt *= self.chemistry.coul_eff
            ddq_dt -= self.chemistry.dqdt*self.q_capacity*temp_rate
            self.Q = self.scaler_q.calculate(ddq_dt)
            self.R = self.scaler_r.calculate(ddq_dt)
            self.Q = Battery.EKF_Q_SD_NORM**2  # override
            self.R = Battery.EKF_R_SD_NORM**2  # override
            # if reset and x_old is not None:
            if x_old is not None:
                self.x_ekf = x_old
            self.predict_ekf(u=ddq_dt, u_old=u_old)  # u = d(q)/dt
            self.update_ekf(z=self.voc_stat, x_min=0., x_max=1., z_old=z_old)  # z = voc, voc_filtered = hx
            self.soc_ekf = self.x_ekf  # x = Vsoc (0-1 ideal capacitor voltage) proxy for soc
            self.q_ekf = self.soc_ekf * self.q_capacity
            self.y_filt = self.y_filt_lag.calculate(in_=self.y_ekf, dt=min(self.dt_eframe, Battery.EKF_T_RESET),
                                                    reset=False)
            self.y_filt2 = self.y_filt_2Ord.calculate(in_=self.y_ekf, dt=min(self.dt_eframe, Battery.TMAX_FILT),
                                                      reset=False)
            # EKF convergence
            conv = abs(self.y_filt) < Battery.EKF_CONV
            self.EKF_converged.calculate(conv, Battery.EKF_T_CONV, Battery.EKF_T_RESET,
                                         min(self.dt_eframe, Battery.EKF_T_RESET), False)
        self.eframe += 1
        if reset or self.eframe >= self.eframe_mult:  # '>=' ensures reset with changes on the fly
            self.eframe = 0

        # Filtered voc
        self.voc_filt = self.sdb_voc.update_res(self.voc, reset)

        # Charge time
        if self.ib_charge > 0.1:
            self.tcharge_ekf = min(Battery.UNIT_CAP_RATED/self.ib_charge * (1. - self.soc_ekf), 24.)
        elif self.ib_charge < -0.1:
            self.tcharge_ekf = max(Battery.UNIT_CAP_RATED/self.ib_charge * self.soc_ekf, -24.)
        elif self.ib_charge >= 0.:
            self.tcharge_ekf = 24.*(1. - self.soc_ekf)
        else:
            self.tcharge_ekf = -24.*self.soc_ekf

        self.dv_dyn = self.dv_dyn
        self.voc_ekf = self.hx
        self.ib_past = self.ib

        return self.vb_model_rev

    def calc_charge_time(self, q, q_capacity, charge_curr, soc):
        delta_q = q - q_capacity
        if charge_curr > Battery.TCHARGE_DISPLAY_DEADBAND:
            self.tcharge = min(-delta_q / charge_curr / 3600., 24.)
        elif charge_curr < -Battery.TCHARGE_DISPLAY_DEADBAND:
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
            self.amp_hrs_remaining_ekf = amp_hrs_remaining * (self.soc_ekf - self.soc_min) / (soc - self.soc_min)
            self.amp_hrs_remaining_wt = amp_hrs_remaining * (self.soc - self.soc_min) / (soc - self.soc_min)
        else:
            self.amp_hrs_remaining_ekf = 0.
            self.amp_hrs_remaining_wt = 0.
        return self.tcharge

    # def count_coulombs(self, dt=0., reset=False, temp_c=25., charge_curr=0., sat=True):
    #     raise NotImplementedError

    def converged_ekf(self):
        return self.EKF_converged.state()

    def ekf_predict(self):
        """Process model"""
        self.Fx = 1. - self.dt_eframe / self.chemistry.tau_sd
        self.Bu = self.dt_eframe / self.chemistry.tau_sd * self.chemistry.r_sd
        return self.Fx, self.Bu

    def ekf_update(self):
        # Measurement function hx(x), x = soc ideal capacitor
        x_lim = max(min(self.x_ekf, 1.), 0.)
        self.hx, self.dv_dsoc = self.calc_soc_voc(x_lim, temp_c=self.temp_c)
        # Jacobian of measurement function
        self.H = self.dv_dsoc
        return self.hx, self.H

    def lag_ib(self, ib, reset):
        self.ib_lag = self.IbLag.calculate_tau(ib, reset, self.dt, self.chemistry.ib_lag_tau)

    def init_soc_ekf(self, soc):
        self.soc_ekf = soc
        self.init_ekf(soc, 0.0)
        self.q_ekf = self.soc_ekf * self.q_capacity

    def regauge(self, temp_c):
        if self.converged_ekf() and abs(self.soc_ekf - self.soc) > Battery.DF2:
            print("Resetting Coulomb Counter Monitor from ", self.soc, " to EKF=", self.soc_ekf, "...")
            self.apply_soc(self.soc_ekf, temp_c)
            print("confirmed ", self.soc)

    def save(self, time, dt, soc_ref, voc_ref):  # BatteryMonitor
        self.saved.time.append(time)
        self.saved.time_min.append(time / 60.)
        self.saved.time_day.append(time / 3600. / 24.)
        self.saved.dt.append(dt)
        self.saved.chm.append(self.chm)
        self.saved.qcrs.append(self.q_cap_rated_scaled)
        self.saved.ib.append(self.ib)
        self.saved.ib_in.append(self.ib_in)
        self.saved.ib_charge.append(self.ib_charge)
        self.saved.ioc.append(self.ioc)
        self.saved.vb.append(self.vb)
        self.saved.dv_hys.append(self.dv_hys)
        self.saved.tau_hys.append(self.tau_hys)
        self.saved.dv_dyn.append(self.dv_dyn)
        self.saved.voc.append(self.voc)
        self.saved.voc_soc.append(self.voc_soc)
        self.saved.voc_stat.append(self.voc_stat)
        self.saved.soc.append(self.soc)
        self.saved.soc_ekf.append(self.soc_ekf)
        self.saved.Fx.append(self.Fx)
        self.saved.Bu.append(self.Bu)
        self.saved.P.append(self.P)
        self.saved.Q.append(self.Q)
        self.saved.R.append(self.R)
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
        self.saved.Tb.append(self.temp_c)
        self.saved.vsat.append(self.vsat)
        self.saved.voc_ekf.append(self.voc_ekf)
        self.saved.sat.append(int(self.sat))
        self.saved.sel.append(self.sel)
        self.saved.mod_data.append(self.mod)
        self.saved.soc_s.append(self.soc_s)
        self.saved.bms_off.append(self.bms_off)
        self.saved.reset.append(self.reset)
        self.saved.e_wrap.append(self.e_wrap)
        self.saved.e_wrap_filt.append(self.e_wrap_filt)
        self.saved.ib_lag.append(self.ib_lag)
        self.saved.voc_soc_new.append(self.voc_soc_new)


class BatterySim(Battery):
    """Extend Battery class to make a model"""

    def __init__(self, q_cap_rated=Battery.UNIT_CAP_RATED*3600, t_rated=25., temp_rlim=0.017, scale=1., stauct=1.,
                 temp_c=25., hys_scale=1., tweak_test=False, dv_hys=0., sres0=1., sresct=1., scale_r_ss=1.,
                 s_hys=1., dvoc=0., scale_hys_cap=1., mod_code=0, s_cap_chg=1., s_cap_dis=1., s_hys_chg=1.,
                 s_hys_dis=1., s_coul_eff=1., cutback_gain_sclr=1., ds_voc_soc=0.):
        Battery.__init__(self, q_cap_rated=q_cap_rated, t_rated=t_rated, temp_rlim=temp_rlim, temp_c=temp_c,
                         tweak_test=tweak_test, sres0=sres0, sresct=sresct, stauct=stauct, scale_r_ss=scale_r_ss,
                         s_hys=s_hys, dvoc=dvoc, mod_code=mod_code, s_coul_eff=s_coul_eff, scale_cap=scale)
        self.lut_voc = None
        self.sat_ib_max = 0.  # Current cutback to be applied to modeled ib output, A
        # self.sat_ib_null = 0.1*Battery.UNIT_CAP_RATED  # Current cutback value for voc=vsat, A
        self.sat_ib_null = 0.  # Current cutback value for soc=1, A
        # self.sat_cutback_gain = 4.8  # Gain to retard ib when voc exceeds vsat, dimensionless
        self.sat_cutback_gain = 1000.*cutback_gain_sclr  # Gain to retard ib when soc approaches 1, dimensionless
        self.ds_voc_soc = ds_voc_soc
        self.model_cutback = False  # Indicate current being limited on saturation cutback, T = cutback limited
        self.model_saturated = False  # Indicator of maximal cutback, T = cutback saturated
        self.ib_sat = 0.5  # Threshold to declare saturation.  This regeneratively slows down charging so if too
        # small takes too long, A
        self.s_cap = scale  # Rated capacity scalar
        if scale is not None:
            self.apply_cap_scale(scale)
        self.hys = Hysteresis(scale=hys_scale, dv_hys=dv_hys, scale_cap=scale_hys_cap, s_cap_chg=s_cap_chg,
                              s_cap_dis=s_cap_dis, s_hys_chg=s_hys_chg, s_hys_dis=s_hys_dis, chem=self.chem,
                              chemistry=self.chemistry)  # Battery hysteresis model - drift of voc
        self.tweak_test = tweak_test
        self.voc = 0.  # Charging voltage, V
        self.d_delta_q = 0.  # Charging rate, Coulombs/sec
        self.charge_curr = 0.  # Charge current, A
        self.saved_s = SavedS()  # for plots and prints
        self.ib_fut = 0.  # Future value of limited current, A

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix + "BatterySim:\n"
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
        s += self.hys.__str__(prefix + 'BatterySim:')
        s += "  \n  "
        s += Battery.__str__(self, prefix + 'BatterySim:')
        return s

    # BatterySim::calculate()
    def calculate(self, chem, temp_c, soc, curr_in, dt, q_capacity, dc_dc_on, reset, update_time_in,  # BatterySim
                  rp=None, sat_init=None, bms_off_init=None, nS=1):
        if self.chm != chem:
            self.chemistry.assign_all_mod(chem)
            self.chm = chem

        self.temp_c = temp_c
        self.dt = dt
        self.ib_in = curr_in
        if reset:
            self.ib_fut = self.ib_in
        self.ib = max(min(self.ib_fut, Battery.IMAX_NUM), -Battery.IMAX_NUM)
        self.mod = rp.modeling
        soc_lim = max(min(soc, 1.), -0.2)  # dag 9/3/2022

        # VOC-OCV model
        self.voc_stat, self.dv_dsoc = self.calc_soc_voc(soc + Battery.D_SOC_S, temp_c)
        # slightly beyond but don't windup
        self.voc_stat = min(self.voc_stat + (soc - soc_lim) * self.dv_dsoc, self.vsat * 1.2)

        # Hysteresis model
        self.hys.calculate_hys(curr_in, self.soc, self.chm)
        init_low = self.bms_off or (self.soc < (self.soc_min + Battery.HYS_SOC_MIN_MARG) and
                                    self.ib > Battery.HYS_IB_THR)
        self.dv_hys, self.tau_hys = self.hys.update(self.dt, init_high=self.sat, init_low=init_low, e_wrap=0.,
                                                    chem=self.chm)
        self.voc = self.voc_stat + self.dv_hys
        self.ioc = self.hys.ioc

        # Battery management system (bms)   I believe bms can see only vb but using this for a model causes
        # lots of chatter as it shuts off, restores vb due to loss of dynamic current, then repeats shutoff.
        # Using voc_ is not better because change in dv_hys_ causes the same effect.   So using nice quiet
        # voc_stat_ for ease of simulation, not accuracy.
        if not self.bms_off:
            voltage_low = self.voc_stat < self.chemistry.vb_down_sim
        else:
            voltage_low = self.voc_stat < self.chemistry.vb_rising_sim
        bms_charging = self.ib_in > Battery.IB_MIN_UP
        if reset and bms_off_init is not None:
            self.bms_off = bms_off_init
        else:
            self.bms_off = (self.temp_c < self.chemistry.low_t) or (voltage_low and not rp.tweak_test())
        ib_charge_fut = self.ib_in / nS
        if self.bms_off and not bms_charging:
            ib_charge_fut = 0.
        if self.bms_off and voltage_low:
            self.ib = 0.
        self.ib_lag = self.IbLag.calculate_tau(self.ib, reset, self.dt, self.chemistry.ib_lag_tau)

        # Charge transfer dynamics
        self.vb = self.voc + (self.ChargeTransfer.calculate(self.ib, reset, dt)*self.chemistry.r_ct +
                              self.ib*self.chemistry.r_0)
        if self.bms_off:
            self.vb = self.voc
        if self.bms_off and dc_dc_on:
            self.vb = Battery.VB_DC_DC
        self.dv_dyn = self.vb - self.voc

        # Saturation logic, both full and empty
        self.vsat = self.chemistry.nom_vsat + (temp_c - 25.) * self.chemistry.dvoc_dt
        self.sat_ib_max = (self.sat_ib_null + (1 - self.soc - self.ds_voc_soc) * self.sat_cutback_gain *
                           rp.cutback_gain_scalar)
        if rp.tweak_test() or (not rp.modeling):
            self.sat_ib_max = ib_charge_fut
        self.ib_fut = min(ib_charge_fut, self.sat_ib_max) * nS  # the feedback of self.ib
        # self.ib_charge = ib_charge_fut / nS # same time plane as volt calcs.  (This prevents sat logic from working)
        self.ib_charge = self.ib_fut / nS  # same time plane as volt calcs
        if self.mod > 0.:
            if (self.q <= 0.) & (self.ib_charge < 0.):
                print("q", self.q, "empty")
                self.ib_charge = 0.  # empty
        self.model_cutback = (self.voc_stat > self.vsat) & (self.ib_fut == self.sat_ib_max)
        self.model_saturated = self.model_cutback & (self.ib_fut < self.ib_sat)
        if reset and sat_init is not None:
            self.model_saturated = sat_init
            self.sat = sat_init
        self.sat = self.model_saturated

        return self.vb

    def count_coulombs(self, chem, dt, reset, temp_c, charge_curr, sat, soc_s_init=None, mon_delta_q=None, mon_sat=None,
                       use_soc_in=False, soc_in=0.):
        # BatterySim
        """Coulomb counter based on true=actual capacity
        Internal resistance of battery is a loss
        Inputs:
            dt              Integration step, s
            temp_c          Battery temperature, deg C
            charge_curr     Charge, A
            sat             Indicator that battery is saturated (VOC>threshold(temp)), T/F
            t_last          Past value of battery temperature used for rate limit memory, deg C
            coul_eff_       Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs
            use_soc_in      Command to drive integrator with input mon_soc
            soc_in          Auxiliary integrator setting, fraction soc
        Outputs:
            soc     State of charge, fraction (0-1.5)
        """
        if self.chm != chem:
            self.chemistry.assign_all_mod(chem)
            self.chm = chem
        self.reset = reset
        self.charge_curr = charge_curr
        self.d_delta_q = self.charge_curr * dt
        if self.charge_curr > 0. and not self.tweak_test:
            self.d_delta_q *= self.chemistry.coul_eff

        # Rate limit temperature
        self.temp_lim = max(min(temp_c, self.t_last + self.temp_rlim*dt), self.t_last - self.temp_rlim*dt)
        if self.reset:
            self.temp_lim = temp_c
            self.t_last = temp_c
            if soc_s_init and not self.mod:
                self.delta_q = self.calculate_capacity(temp_c) * (soc_s_init - 1.)

        # Saturation.   Goal is to set q_capacity and hold it so remembers last saturation status
        # detection
        if not self.mod and mon_sat:
            self.delta_q = mon_delta_q
        if self.model_saturated:
            if reset:
                self.delta_q = 0.  # Model is truth.   Saturate it then restart it to reset charge
        self.resetting = False  # one pass flag.  Saturation debounce should reset next pass

        # Integration can go to -50%
        self.q_capacity = self.calculate_capacity(self.temp_lim)
        if use_soc_in:
            self.soc = soc_in
            self.q = self.q_capacity * self.soc
            self.delta_q = self.q - self.q_capacity
        else:
            self.delta_q += self.d_delta_q - self.chemistry.dqdt*self.q_capacity*(self.temp_lim-self.t_last)
            self.delta_q = max(min(self.delta_q, 0.), -self.q_capacity*1.5)
            self.q = self.q_capacity + self.delta_q

        # Normalize
        self.soc = self.q / self.q_capacity
        self.soc_min = self.chemistry.lut_min_soc.interp(self.temp_lim)
        self.q_min = self.soc_min * self.q_capacity

        # Save and return
        self.t_last = self.temp_lim
        return self.soc

    def save(self, time, dt):  # BatterySim
        self.saved.time.append(time)
        self.saved.dt.append(dt)
        self.saved.ib.append(self.ib)
        self.saved.ib_in.append(self.ib_in)
        self.saved.ib_charge.append(self.ib_charge)
        self.saved.chm.append(self.chm)
        self.saved.bmso.append(self.bms_off)
        self.saved.ioc.append(self.ioc)
        self.saved.vb.append(self.vb)
        self.saved.dv_hys.append(self.dv_hys)
        self.saved.tau_hys.append(self.tau_hys)
        self.saved.dv_dyn.append(self.dv_dyn)
        self.saved.voc.append(self.voc)
        self.saved.voc_stat.append(self.voc_stat)
        self.saved.soc.append(self.soc)
        self.saved.d_delta_q.append(self.d_delta_q)
        self.saved.Tb.append(self.temp_c)
        self.saved.t_last.append(self.t_last)
        self.saved.vsat.append(self.vsat)
        self.saved.charge_curr.append(self.charge_curr)
        self.saved.sat.append(int(self.model_saturated))
        self.saved.delta_q.append(self.delta_q)
        self.saved.q.append(self.q)
        self.saved.q_capacity.append(self.q_capacity)
        self.saved.bms_off.append(self.bms_off)

    def save_s(self, time):
        self.saved_s.time.append(time)
        self.saved_s.chm_s.append(self.chm)
        self.saved_s.qcrs_s.append(self.q_cap_rated_scaled)
        self.saved_s.bmso_s.append(self.bms_off)
        self.saved_s.Tb_s.append(self.temp_c)
        self.saved_s.Tbl_s.append(self.temp_lim)
        self.saved_s.vsat_s.append(self.vsat)
        self.saved_s.voc_s.append(self.voc)
        self.saved_s.voc_stat_s.append(self.voc_stat)
        self.saved_s.dv_dyn_s.append(self.dv_dyn)
        self.saved_s.dv_hys_s.append(self.dv_hys)
        self.saved_s.tau_hys_s.append(self.tau_hys)
        self.saved_s.vb_s.append(self.vb)
        self.saved_s.ib_s.append(self.ib)
        self.saved_s.ib_in_s.append(self.ib_in)
        self.saved_s.ib_charge_s.append(self.ib_charge)
        self.saved_s.ib_fut_s.append(self.ib_fut)
        self.saved_s.sat_s.append(int(self.sat))
        self.saved_s.dq_s.append(self.delta_q)
        self.saved_s.soc_s.append(self.soc)
        self.saved_s.reset_s.append(self.reset)
        self.saved_s.tau_s.append(self.tau_hys)


# Other functions
def is_sat(temp_c, voc, soc, nom_vsat, dvoc_dt, low_t):
    vsat = sat_voc(temp_c, nom_vsat, dvoc_dt)
    return temp_c > low_t and (voc >= vsat or soc >= Battery.mxeps_bb)


def calculate_capacity(temp_c, t_sat, q_sat, dqdt):
    return q_sat * (1-dqdt*(temp_c - t_sat))


def calc_vsat(temp_c, vsat, dvoc_dt):
    return sat_voc(temp_c, vsat, dvoc_dt)


def sat_voc(temp_c, vsat, dvoc_dt):
    return vsat + (temp_c-25.)*dvoc_dt


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.time_min = []
        self.time_day = []
        self.dt = []
        self.chm = []
        self.qcrs = []
        self.bmso = []
        self.ib = []
        self.ib_in = []
        self.ib_charge = []
        self.ioc = []
        self.vb = []
        self.voc = []
        self.voc_soc = []
        self.voc_stat = []
        self.dv_hys = []
        self.tau_hys = []
        self.dv_dyn = []
        self.soc = []
        self.soc_ekf = []
        self.voc = []
        self.Fx = []
        self.Bu = []
        self.P = []
        self.Q = []
        self.R = []
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
        self.ib = []  # Bank current, A
        self.vb = []  # Bank voltage, V
        self.sat = []  # Indication that battery is saturated, T=saturated
        self.sel = []  # Current source selection, 0=amp, 1=no amp
        self.mod_data = []  # Configuration control code, 0=all hardware, 7=all simulated, +8 tweak test
        self.Tb = []  # Battery bank temperature, deg C
        self.vsat = []  # Monitor Bank saturation threshold at temperature, deg C
        self.dv_dyn = []  # Monitor Bank current induced back emf, V
        self.voc_stat = []  # Monitor Static bank open circuit voltage, V
        self.voc = []  # Monitor Static bank open circuit voltage, V
        self.voc_ekf = []  # Monitor bank solved static open circuit voltage, V
        self.y_ekf = []  # Monitor single battery solver error, V
        self.y_filt = []  # Filtered EKF y residual value, V
        self.y_filt2 = []  # Filtered EKF y residual value, V
        self.soc_s = []  # Simulated state of charge, fraction
        self.soc_ekf = []  # Solved state of charge, fraction
        # self.soc = []  # Coulomb Counter fraction of saturation charge (q_capacity_) available (0-1)
        self.d_delta_q = []  # Charging rate, Coulombs/sec
        self.charge_curr = []  # Charging current, A
        self.q = []  # Present charge available to use, except q_min_, C
        self.delta_q = []  # Charge change since saturated, C
        self.q_capacity = []  # Saturation charge at temperature, C
        self.t_last = []  # Past value of battery temperature used for rate limit memory, deg C
        self.bms_off = []  # Voltage low without faults, battery management system has shut off battery
        self.reset = []  # Reset flag used for initialization
        self.e_wrap = []  # Verification of wrap calculation, V
        self.e_wrap_filt = []  # Verification of filtered wrap calculation, V
        self.ib_lag = []  # Lagged ib, A
        self.voc_soc_new = []  # New schedule values


def overall_batt(mv, sv, filename,
                 mv1=None, sv1=None, suffix1=None, fig_files=None, plot_title=None, fig_list=None, suffix='',
                 use_time_day=False):
    if fig_files is None:
        fig_files = []

    if mv1 is None:
        if use_time_day:
            mv.time = mv.time_day - mv.time_day[0]
            sv.time = sv.time_day - sv.time_day[0]

        plt.figure()  # Batt 1
        fig_list += 1
        plt.subplot(321)
        plt.title(plot_title + ' B 1')
        plt.plot(mv.time, mv.ib, color='green',   linestyle='-', label='ib'+suffix)
        plt.plot(mv.time, mv.ioc, color='magenta', linestyle='--', label='ioc'+suffix)
        plt.legend(loc=1)
        plt.subplot(323)
        plt.plot(mv.time, mv.vb, color='green', linestyle='-', label='vb'+suffix)
        plt.plot(sv.time, sv.vb, color='black', linestyle='--', label='vb_s'+suffix)
        plt.plot(mv.time, mv.voc_stat, color='orange', linestyle='-.', label='voc_stat'+suffix)
        plt.plot(sv.time, sv.voc_stat, color='cyan', linestyle=':', label='voc_stat_s,'+suffix)
        plt.plot(mv.time, mv.voc, color='magenta', label='voc'+suffix)
        plt.plot(sv.time, sv.voc, color='black', linestyle='--', label='voc_s'+suffix)
        plt.legend(loc=1)
        plt.subplot(324)
        # plt.legend(loc=1)
        plt.subplot(322)
        plt.plot(mv.time, mv.soc, color='red', linestyle='-', label='soc'+suffix)
        plt.plot(sv.time, sv.soc, color='black', linestyle='dotted', label='soc_s'+suffix)
        plt.legend(loc=1)
        plt.subplot(325)
        plt.plot(mv.time, mv.chm, color='cyan', linestyle='--', label='chm'+suffix)
        plt.plot(sv.time, sv.chm, color='black', linestyle=':', label='chm_s'+suffix)
        plt.legend(loc=1)
        plt.subplot(326)
        plt.plot(sv.soc, sv.voc, color='red', linestyle='-', label='SIM voc_stat vs soc'+suffix)
        plt.plot(mv.soc, mv.voc_soc, color='black', linestyle='--', label='MON voc_soc'+suffix+' vs soc')
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 2
        fig_list += 1
        plt.subplot(111)
        plt.title(plot_title + ' B 2')
        plt.plot(mv.time, mv.vb, color='green', linestyle='-', label='vb'+suffix)
        plt.plot(sv.time, sv.vb, color='black', linestyle='--', label='vb_s'+suffix)
        plt.plot(mv.time, mv.voc_stat, color='orange', linestyle='-.', label='voc_stat'+suffix)
        plt.plot(sv.time, sv.voc_stat, color='cyan', linestyle=':', label='voc_stat'+suffix)
        plt.plot(mv.time, mv.voc, color='magenta', linestyle='-', label='voc'+suffix)
        plt.plot(sv.time, sv.voc, color='black', linestyle='--', label='voc_s'+suffix)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 4
        fig_list += 1
        plt.subplot(321)
        plt.title(plot_title+' B 4 MON vs SIM')
        plt.plot(mv.time, mv.ib, color='green', linestyle='-', label='ib'+suffix)
        plt.plot(sv.time, sv.ib, color='black', linestyle='--', label='ib_s'+suffix)
        plt.plot(sv.time, sv.ib_in, color='red', linestyle='-.', label='ib_in_s'+suffix)
        plt.legend(loc=1)
        plt.subplot(322)
        plt.plot(mv.time, mv.vb, color='green', linestyle='-', label='vb'+suffix)
        plt.plot(sv.time, sv.vb, color='black', linestyle='--', label='vb_s'+suffix)
        plt.plot(mv.time, mv.voc, color='cyan', linestyle='-', label='voc'+suffix)
        plt.plot(sv.time, sv.voc, color='red', linestyle='--', label='voc_s'+suffix)
        plt.legend(loc=1)
        plt.subplot(323)
        plt.plot(mv.time, mv.vb, color='green', linestyle='-', label='vb'+suffix)
        plt.plot(sv.time, sv.vb, color='orange', linestyle='--', label='vb_s'+suffix)
        plt.plot(mv.time, mv.voc, color='cyan', linestyle='-.', label='voc'+suffix)
        plt.plot(sv.time, sv.voc, color='red', linestyle=':', label='voc_s'+suffix)
        plt.plot(mv.time, mv.voc_stat, color='magenta', linestyle='--', label='voc_stat'+suffix)
        plt.plot(sv.time, sv.voc_stat, color='black', linestyle=':', label='voc_stat_s'+suffix)
        plt.legend(loc=1)
        plt.subplot(324)
        plt.plot(mv.time, mv.dv_dyn, color='green', linestyle='-', label='dv_dyn'+suffix)
        plt.plot(sv.time, sv.dv_dyn, color='black', linestyle='--', label='dv_dyn_s'+suffix)
        plt.legend(loc=1)
        plt.subplot(325)
        plt.plot(mv.time, mv.dv_hys, color='green', linestyle='-', label='dv_hys'+suffix)
        plt.plot(sv.time, sv.dv_hys, color='black', linestyle='--', label='dv_hys_s'+suffix)
        plt.plot(mv.time, mv.tau_hys, color='cyan', linestyle='-', label='tau_hys'+suffix)
        plt.plot(sv.time, sv.tau_hys, color='red', linestyle='--', label='tau_hys_s'+suffix)
        plt.legend(loc=1)
        plt.subplot(326)
        plt.plot(mv.time, mv.vb, color='green', linestyle='-', label='vb'+suffix)
        plt.plot(sv.time, sv.vb, color='black', linestyle='--', label='vb_s'+suffix)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 5
        fig_list += 1
        plt.subplot(331)
        plt.title(plot_title+' B 5 **EKF')
        plt.plot(mv.time, mv.x_ekf, color='red', linestyle='-', label='x ekf'+suffix)
        plt.legend(loc=4)
        plt.subplot(332)
        plt.plot(mv.time, mv.hx, color='cyan', linestyle='-', label='hx ekf'+suffix)
        plt.plot(mv.time, mv.z_ekf, color='black', linestyle='--', label='z ekf'+suffix)
        plt.legend(loc=4)
        plt.subplot(333)
        plt.plot(mv.time, mv.y_ekf, color='green', linestyle='-', label='y ekf'+suffix)
        plt.plot(mv.time, mv.y_filt, color='black', linestyle='--', label='y filt'+suffix)
        plt.plot(mv.time, mv.y_filt2, color='cyan', linestyle='-.', label='y filt2'+suffix)
        plt.legend(loc=4)
        plt.subplot(334)
        plt.plot(mv.time, mv.H, color='magenta', linestyle='-', label='H ekf'+suffix)
        plt.ylim(0, 150)
        plt.legend(loc=3)
        plt.subplot(335)
        plt.plot(mv.time, mv.P, color='orange', linestyle='-', label='P ekf'+suffix)
        plt.legend(loc=3)
        plt.subplot(336)
        plt.plot(mv.time, mv.Fx, color='red', linestyle='-', label='Fx ekf'+suffix)
        plt.legend(loc=2)
        plt.subplot(337)
        plt.plot(mv.time, mv.Bu, color='blue', linestyle='-', label='Bu ekf'+suffix)
        plt.legend(loc=2)
        plt.subplot(338)
        plt.plot(mv.time, mv.K, color='red', linestyle='-', label='K ekf'+suffix)
        plt.legend(loc=4)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 6
        fig_list += 1
        plt.title(plot_title + ' B 6')
        plt.plot(mv.time, mv.e_voc_ekf, color='blue', linestyle='-.', label='e_voc'+suffix)
        plt.plot(mv.time, mv.e_soc_ekf, color='red', linestyle='dotted', label='e_soc_ekf'+suffix)
        plt.ylim(-0.01, 0.01)
        plt.legend(loc=2)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 7
        fig_list += 1
        plt.title(plot_title + ' B 7')
        plt.plot(mv.time, mv.voc, color='red', linestyle='-', label='voc'+suffix)
        plt.plot(mv.time, mv.voc_ekf, color='blue', linestyle='-.', label='voc_ekf'+suffix)
        plt.plot(sv.time, sv.voc, color='green', linestyle=':', label='voc_s'+suffix)
        plt.legend(loc=4)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 8
        fig_list += 1
        plt.title(plot_title + ' B 8')
        plt.plot(mv.time, mv.soc_ekf, color='blue', linestyle='-', label='soc_ekf'+suffix)
        plt.plot(sv.time, sv.soc, color='green', linestyle='-.', label='soc_s'+suffix)
        plt.plot(mv.time, mv.soc, color='red', linestyle=':', label='soc'+suffix)
        plt.legend(loc=4)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 9
        fig_list += 1
        plt.title(plot_title + ' B 9')
        plt.plot(mv.time, mv.e_voc_ekf, color='blue', linestyle='-.', label='e_voc'+suffix)
        plt.plot(mv.time, mv.e_soc_ekf, color='red', linestyle='dotted', label='e_soc_ekf'+suffix)
        plt.legend(loc=2)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 10
        fig_list += 1
        plt.subplot(221)
        plt.title(plot_title + ' B 10')
        plt.plot(sv.time, sv.soc, color='red', linestyle='-', label='soc'+suffix)
        plt.legend(loc=1)
        plt.subplot(223)
        plt.plot(sv.time, sv.ib, color='blue', linestyle='-', label='ib, A'+suffix)
        plt.plot(sv.time, sv.ioc, color='green', linestyle='-', label='ioc hys indicator, A'+suffix)
        plt.legend(loc=1)
        plt.subplot(224)
        plt.plot(sv.time, sv.dv_hys, color='red', linestyle='-', label='dv_hys, V'+suffix)
        plt.plot(sv.time, sv.tau_hys, color='blue', linestyle='--', label='tau_hys, V'+suffix)
        plt.legend(loc=2)
        fig_file_name = filename + "_" + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 11
        fig_list += 1
        plt.subplot(111)
        plt.title(plot_title + ' B 11')
        plt.plot(sv.soc, sv.voc_stat, color='black', linestyle='dotted', label='SIM voc_stat vs soc'+suffix)
        plt.legend(loc=2)
        fig_file_name = filename + "_" + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

    else:
        if use_time_day:
            mv.time = mv.time_day - mv.time_day[0]
            try:
                sv.time = sv.time_day - sv.time_day[0]
            except IOError:
                pass
            mv1.time = mv1.time_day - mv1.time_day[0]
            try:
                sv1.time = sv1.time_day - sv1.time_day[0]
            except IOError:
                pass
        reset_max = max(abs(min(mv.vbc_dot)), max(mv.vbc_dot), abs(min(mv1.vbc_dot)), max(mv1.vbc_dot))
        # noinspection PyTypeChecker
        reset_index_max = max(np.where(np.array(mv1.reset) > 0))
        t_init = mv1.time[reset_index_max[-1]]
        mv.time -= t_init
        mv1.time -= t_init
        sv.time -= t_init
        sv1.time -= t_init

        plt.figure()
        fig_list += 1
        plt.subplot(331)
        plt.title(plot_title + ' Battover 1')
        plt.plot(mv.time, mv.ib, color='green',   linestyle='-', label='ib'+suffix)
        plt.plot(mv1.time, mv1.ib, color='black', linestyle='--', label='ib' + suffix1)
        plt.plot(mv.time, mv.ioc, color='magenta', linestyle='-.', label='ioc'+suffix)
        plt.plot(mv1.time, mv1.ioc, color='blue', linestyle=':', label='ioc' + suffix1)
        plt.legend(loc=1)
        plt.subplot(332)
        # plt.legend(loc=1)
        plt.subplot(333)
        # plt.legend(loc=1)
        plt.subplot(334)
        plt.plot(mv.time, mv.vb, color='green', linestyle='-', label='vb' + suffix)
        plt.plot(mv1.time, mv1.vb, color='black', linestyle='--', label='vb' + suffix1)
        plt.legend(loc=1)
        plt.subplot(335)
        plt.plot(mv.time, mv.voc_stat, color='magenta', linestyle='-.', label='voc_stat' + suffix)
        plt.plot(mv1.time, mv1.voc_stat, color='blue', linestyle=':', label='voc_stat' + suffix1)
        plt.legend(loc=1)
        plt.subplot(336)
        plt.plot(mv.time, mv.voc, color='green', linestyle='-', label='voc' + suffix)
        plt.plot(mv1.time, mv1.voc, color='black', linestyle='--', label='voc' + suffix1)
        plt.legend(loc=1)
        plt.subplot(337)
        plt.plot(mv.time, mv.vbc_dot, color='green', linestyle='-', label='vbc_dot' + suffix)
        plt.plot(mv1.time, mv1.vbc_dot, color='black', linestyle='--', label='vbc_dot' + suffix1)
        plt.plot(mv.time, np.array(mv.reset)*reset_max, color='orange', linestyle='-', label='reset'+suffix)
        plt.plot(mv1.time, np.array(mv1.reset)*reset_max, color='cyan', linestyle='--', label='reset'+suffix1)
        plt.legend(loc=1)
        plt.subplot(338)
        plt.plot(mv.time, mv.vcd_dot, color='green', linestyle='-', label='vcd_dot' + suffix)
        plt.plot(mv1.time, mv1.vcd_dot, color='black', linestyle='--', label='vcd_dot' + suffix1)
        plt.legend(loc=1)
        plt.subplot(339)
        plt.plot(mv.time, mv.soc, color='green', linestyle='-', label='soc' + suffix)
        plt.plot(mv1.time, mv1.soc, color='black', linestyle='--', label='soc' + suffix1)
        plt.plot(mv.time, mv.soc_ekf, color='magenta', linestyle='-.', label='soc_ekf'+suffix)
        plt.plot(mv1.time, mv1.soc_ekf, color='blue', linestyle=':', label='soc_ekf'+suffix1)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()
        fig_list += 1
        plt.subplot(321)
        plt.title(plot_title + ' Battover 2')
        plt.plot(mv.time, mv.ib, color='green',   linestyle='-', label='ib'+suffix)
        plt.plot(mv1.time, mv1.ib, color='black', linestyle='--', label='ib' + suffix1)
        plt.plot(mv.time, mv.ioc, color='magenta', linestyle='-.', label='ioc'+suffix)
        plt.plot(mv1.time, mv1.ioc, color='blue', linestyle=':', label='ioc' + suffix1)
        plt.legend(loc=1)
        plt.subplot(322)
        plt.plot(mv.time, mv.dv_dyn, color='green', linestyle='-', label='dv_dyn'+suffix)
        plt.plot(mv1.time, mv1.dv_dyn, color='black', linestyle='--', label='dv_dyn'+suffix1)
        plt.legend(loc=1)
        plt.subplot(323)
        plt.plot(mv.time, mv.dv_hys, color='green', linestyle='-', label='dv_hys'+suffix)
        plt.plot(mv1.time, mv1.dv_hys, color='black', linestyle='--', label='dv_hys'+suffix1)
        plt.legend(loc=1)
        plt.subplot(324)
        plt.plot(mv.time, mv.tau_hys, color='green', linestyle='-', label='tau_hys'+suffix)
        plt.plot(mv1.time, mv1.tau_hys, color='black', linestyle='--', label='tau_hys'+suffix1)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()
        fig_list += 1
        plt.subplot(331)
        plt.title(plot_title + ' **EKF' + 'Battover 3')
        plt.plot(mv.time, mv.x_ekf, color='green', linestyle='-', label='x ekf' + suffix)
        plt.plot(mv1.time, mv1.x_ekf, color='black', linestyle='--', label='x ekf' + suffix1)
        plt.legend(loc=4)
        plt.subplot(332)
        plt.plot(mv.time, mv.hx, color='green', linestyle='-', label='hx ekf' + suffix)
        plt.plot(mv1.time, mv1.hx, color='black', linestyle='--', label='hx ekf' + suffix1)
        plt.plot(mv.time, mv.z_ekf, color='magenta', linestyle='-.', label='z ekf' + suffix)
        plt.plot(mv1.time, mv1.z_ekf, color='blue', linestyle=':', label='z ekf' + suffix1)
        plt.legend(loc=4)
        plt.subplot(333)
        plt.plot(mv.time, mv.y_ekf, color='green', linestyle='-', label='y ekf' + suffix)
        plt.plot(mv1.time, mv1.y_ekf, color='black', linestyle='--', label='y ekf' + suffix1)
        plt.plot(mv.time, mv.y_filt2, color='magenta', linestyle='-.', label='y filt2' + suffix)
        plt.plot(mv1.time, mv1.y_filt2, color='blue', linestyle=':', label='y filt2' + suffix1)
        plt.legend(loc=4)
        plt.subplot(334)
        plt.plot(mv.time, mv.H, color='green', linestyle='-', label='H ekf' + suffix)
        plt.plot(mv1.time, mv1.H, color='black', linestyle='--', label='H ekf' + suffix1)
        plt.ylim(0, 150)
        plt.legend(loc=3)
        plt.subplot(335)
        plt.plot(mv.time, mv.P, color='green', linestyle='-', label='P ekf' + suffix)
        plt.plot(mv1.time, mv1.P, color='black', linestyle='--', label='P ekf' + suffix1)
        plt.legend(loc=3)
        plt.subplot(336)
        plt.plot(mv.time, mv.Fx, color='green', linestyle='-', label='Fx ekf' + suffix)
        plt.plot(mv1.time, mv1.Fx, color='black', linestyle='--', label='Fx ekf' + suffix1)
        plt.legend(loc=2)
        plt.subplot(337)
        plt.plot(mv.time, mv.Bu, color='green', linestyle='-', label='Bu ekf' + suffix)
        plt.plot(mv1.time, mv1.Bu, color='black', linestyle='--', label='Bu ekf' + suffix1)
        plt.legend(loc=2)
        plt.subplot(338)
        plt.plot(mv.time, mv.K, color='green', linestyle='-', label='K ekf' + suffix)
        plt.plot(mv1.time, mv1.K, color='black', linestyle='--', label='K ekf' + suffix1)
        plt.legend(loc=4)
        plt.subplot(339)
        plt.plot(mv.time, mv.e_voc_ekf, color='green', linestyle='-', label='e_voc' + suffix)
        plt.plot(mv1.time, mv1.e_voc_ekf, color='black', linestyle='--', label='e_voc' + suffix1)
        plt.plot(mv.time, mv.e_soc_ekf, color='magenta', linestyle='-.', label='e_soc_ekf' + suffix)
        plt.plot(mv1.time, mv1.e_soc_ekf, color='blue', linestyle=':', label='e_soc_ekf' + suffix1)
        # plt.ylim(-0.01, 0.01)
        plt.legend(loc=2)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

    return fig_list, fig_files


class SavedS:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.time_min = []
        self.time_day = []
        self.unit = []  # text title
        self.c_time = []  # Control time, s
        self.chm_s = []
        self.qcrs_s = []
        self.bmso_s = []
        self.Tb_s = []
        self.Tbl_s = []
        self.vsat_s = []
        self.voc_s = []
        self.voc_stat_s = []
        self.dv_dyn_s = []
        self.dv_hys_s = []
        self.tau_hys_s = []
        self.tau_s = []
        self.vb_s = []
        self.ib_s = []
        self.ib_in_s = []
        self.ib_charge_s = []
        self.ib_fut_s = []
        self.sat_s = []
        self.ddq_s = []
        self.dq_s = []
        self.q_s = []
        self.qcap_s = []
        self.soc_s = []
        self.reset_s = []

    def __str__(self):
        s = "unit_m,c_time,Tb_s,Tbl_s,vsat_s,voc_stat_s,dv_dyn_s,vb_s,ib_s,sat_s,ddq_s,dq_s,q_s,qcap_s,soc_s,\
        reset_s,tau_s,\n"
        for i in range(len(self.time)):
            s += 'sim,'
            s += "{:13.3f},".format(self.time[i])
            s += "{:5.2f},".format(self.Tb_s[i])
            s += "{:5.2f},".format(self.Tbl_s[i])
            s += "{:8.3f},".format(self.vsat_s[i])
            s += "{:5.2f},".format(self.voc_stat_s[i])
            s += "{:5.2f},".format(self.dv_dyn_s[i])
            s += "{:5.2f},".format(self.vb_s[i])
            s += "{:8.3f},".format(self.ib_s[i])
            s += "{:8.3f},".format(self.ib_in_s[i])
            s += "{:8.3f},".format(self.ib_fut_s[i])
            s += "{:1.0f},".format(self.sat_s[i])
            s += "{:5.3f},".format(self.ddq_s[i])
            s += "{:5.3f},".format(self.dq_s[i])
            s += "{:5.3f},".format(self.qcap_s[i])
            s += "{:7.3f},".format(self.soc_s[i])
            s += "{:d},".format(self.reset_s[i])
            s += "{:7.3f},".format(self.tau_s[i])
            s += "\n"
        return s
