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
from Coulombs import Coulombs, dqdt
from StateSpace import StateSpace
from Hysteresis import Hysteresis
import matplotlib.pyplot as plt
from TFDelay import TFDelay
from myFilters import LagTustin, General2Pole, RateLimit, SlidingDeadband
from Scale import Scale
from numpy.linalg import inv
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


# Battery constants
RATED_TEMP = 25.  # Temperature at RATED_BATT_CAP, deg C
BATT_DVOC_DT = 0.004  # 5/30/2022
""" Change of VOC with operating temperature in range 0 - 50 C (0.004) V/deg C
                            >3.425 V is reliable approximation for SOC>99.7 observed in my prototype around 15-35 C"""
BATT_V_SAT = 13.8  # Normal battery cell saturation for SOC=99.7, V (13.8)
NOM_SYS_VOLT = 12.  # Nominal system output, V, at which the reported amps are used (12)
low_voc = 9.  # Minimum voltage for battery below which BMS shuts off current
# low_t = 8.  # Minimum temperature for valid saturation check, because BMS shuts off battery low.
low_t = 4.  # Minimum temperature for valid saturation check, because BMS shuts off battery low.
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
EKF_Q_SD_NORM = 0.0015  # Standard deviation of normal EKF process uncertainty, V (0.0015)
EKF_R_SD_NORM = 0.5  # Standard deviation of normal EKF state uncertainty, fraction (0-1) (0.5)
IMAX_NUM = 100000.  # Overflow protection since ib past value used
HYS_SOC_MIN_MARG = 0.15  # Add to soc_min to set thr for detecting low endpoint condition for reset of hysteresis
HYS_SOC_MAX = 0.99  # Detect high endpoint condition for reset of hysteresis
HYS_E_WRAP_THR = 0.1  # Detect e_wrap going the other way; need to reset dv_hys at endpoints
HYS_IB_THR = 1.  # Ignore reset if opposite situation exists
IB_MIN_UP = 0.2  # Min up charge current for come alive, BMS logic, and fault
V_BATT_OFF = 10.  # Shutoff point in Mon, V (10.)
V_BATT_DOWN_SIM = 9.5  # Shutoff point in Sim before off, V (9.5)
V_BATT_RISING_SIM = 9.75  # Shutoff point in Sim when off, V (9.75)
V_BATT_DOWN = 9.8  # Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V (9.8)
V_BATT_RISING = 10.3  # Shutoff point when off, V (10.3)
RANDLES_T_MAX = 0.31  # Maximum update time of Randles state space model to avoid aliasing and instability (0.31 allows DP3)
cp_eframe_mult = 20  # Run EKF 20 times slower than Coulomb Counter and Randles models
READ_DELAY = 100  # nominal read time, ms
EKF_EFRAME_MULT = 20  # Multiframe rate consistent with READ_DELAY (20 for READ_DELAY=100)
VB_DC_DC = 13.5  # Estimated dc-dc charger, V
HDB_VBATT = 0.05  # Half deadband to filter vb, V (0.05)
WRAP_ERR_FILT = 4.  # Wrap error filter time constant, s (4)
MAX_WRAP_ERR_FILT = 10.  # Anti-windup wrap error filter, V (10)
F_MAX_T_WRAP = 2.8  # Maximum update time of Wrap filter for stability at WRAP_ERR_FILT, s (2.8)
D_SOC_S = 0.  # Bias on soc to voc-soc lookup to simulate error in estimation, esp cold battery near 0 C

class Battery(Coulombs):
    RATED_BATT_CAP = 100.
    # """Nominal battery bank capacity, Ah(100).Accounts for internal losses.This is
    #                         what gets delivered, e.g. Wshunt / NOM_SYS_VOLT.  Also varies 0.2 - 0.4 C currents
    #                         or 20 - 40 A for a 100 Ah battery"""

    # Battery model:  Randles' dynamics, SOC-VOC model

    """Nominal battery bank capacity, Ah(100).Accounts for internal losses.This is
                            what gets delivered, e.g.Wshunt / NOM_SYS_VOLT.  Also varies 0.2 - 0.4 C currents
                            or 20 - 40 A for a 100 Ah battery"""

    def __init__(self, bat_v_sat=13.85, q_cap_rated=RATED_BATT_CAP*3600, t_rated=RATED_TEMP, t_rlim=0.017,
                 r_sd=70., tau_sd=2.5e7, r0=0.003, tau_ct=0.2, r_ct=0.0016, tau_dif=83., r_dif=0.0077,
                 temp_c=RATED_TEMP, tweak_test=False, t_max=RANDLES_T_MAX, sres=1., stauct=1., staudif=1.,
                 scale_r_ss=1., s_hys=1., dvoc=0., chem=0, coul_eff=0.9985):
        """ Default values from Taborelli & Onori, 2013, State of Charge Estimation Using Extended Kalman Filters for
        Battery Management System.   Battery equations from LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang
        etal.pdf.'  SOC-OCV curve fit './Battery State/BattleBorn Rev1.xls:Model Fit' using solver with min slope
        constraint >=0.02 V/soc.  m and n using Zhang values.   Had to scale soc because  actual capacity > NOM_BAT_CAP
        so equation error when soc<=0 to match data.    See Battery.h
        """
        # Parents
        Coulombs.__init__(self, q_cap_rated,  q_cap_rated, t_rated, t_rlim, tweak_test, coul_eff_=coul_eff)

        # Defaults
        self.chem = chem
        from pyDAGx import myTables
        # Battleborn Bmon=0, Bsim=0
        t_x_soc0 = [-0.15, 0.00, 0.05, 0.10, 0.14, 0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99,  0.995, 1.00]
        t_y_t0 = [5.,  11.1,  20.,   30.,   40.]
        t_voc0 = [4.00, 4.00, 4.00,  4.00,  10.20, 11.70, 12.45, 12.70, 12.77, 12.90, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.59, 14.45,
                  4.00, 4.00, 4.00,  9.50,  12.00, 12.50, 12.70, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 13.60, 14.46,
                  4.00, 4.00, 10.00, 12.60, 12.77, 12.85, 12.89, 12.95, 12.99, 13.03, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 13.72, 14.50,
                  4.00, 4.00, 12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26, 13.27, 13.72, 14.50,
                  4.00, 4.00, 4.00,  4.00,  10.50, 11.93, 12.78, 12.83, 12.89, 12.97, 13.06, 13.10, 13.13, 13.16, 13.19, 13.20, 13.72, 14.50]
        x0 = np.array(t_x_soc0)
        y0 = np.array(t_y_t0)
        data_interp0 = np.array(t_voc0)
        self.lut_voc0 = myTables.TableInterp2D(x0, y0, data_interp0)
        # CHINS Bmon=1, Bsim=1
        t_x_soc1 = [-0.15, 0.00, 0.05, 0.10, 0.14,  0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.96,  0.98,  0.99,  1.00]
        t_y_t1 = [5.,  11.1,   25.,   40.]
        t_voc1 = [4.00, 4.00,  4.00,  4.00,  10.00, 11.00, 12.99, 13.05, 13.10, 13.16, 13.18, 13.21, 13.24, 13.30, 13.30, 13.27, 13.27, 13.42, 14.70,
                  4.00, 4.00,  4.00,  9.50,  12.91, 12.95, 12.99, 13.05, 13.10, 13.16, 13.18, 13.21, 13.24, 13.30, 13.30, 13.27, 13.27, 13.42, 14.70,
                  4.00, 4.00,  10.00, 12.87, 12.91, 12.95, 12.99, 13.05, 13.10, 13.16, 13.18, 13.21, 13.24, 13.30, 13.30, 13.27, 13.27, 13.42, 14.70,
                  4.00, 4.00,  11.00, 12.87, 12.91, 12.95, 12.99, 13.05, 13.10, 13.16, 13.18, 13.21, 13.24, 13.30, 13.30, 13.27, 13.27, 13.42, 14.70]

        x1 = np.array(t_x_soc1)
        y1 = np.array(t_y_t1)
        data_interp1 = np.array(t_voc1)
        self.lut_voc1 = myTables.TableInterp2D(x1, y1, data_interp1)
        # LIE Bmon=2
        t_x_soc2 = [-0.15, 0.00, 0.05, 0.10, 0.14, 0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99,  0.995, 1.00]
        t_y_t2 = [5.,  11.1,   25.,   40.]
        t_voc2 = [4.00, 4.00, 4.00,  4.00,  10.20, 11.70, 12.45, 12.70, 12.77, 12.90, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 13.59, 14.45,
                  4.00, 4.00, 8.00,  11.70, 12.50, 12.60, 12.70, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 13.76, 14.46,
                  4.00, 4.00, 10.00, 12.60, 12.77, 12.85, 12.89, 12.95, 12.99, 13.03, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 13.72, 14.50,
                  4.00, 4.00, 10.50, 13.10, 13.27, 13.31, 13.44, 13.46, 13.40, 13.44, 13.48, 13.52, 13.56, 13.60, 13.64, 13.68, 14.22, 15.00]

        x2 = np.array(t_x_soc2)
        y2 = np.array(t_y_t2)
        data_interp2 = np.array(t_voc2)
        self.lut_voc = None
        self.lut_voc2 = myTables.TableInterp2D(x2, y2, data_interp2)
        self.dvoc = dvoc
        self.nz = None
        self.q = 0  # Charge, C
        self.voc = NOM_SYS_VOLT  # Model open circuit voltage, V
        self.voc_stat = self.voc  # Static model open circuit voltage from charge process, V
        self.dv_dyn = 0.  # Model current induced back emf, V
        self.vb = NOM_SYS_VOLT  # Battery voltage at post, V
        self.ib = 0.  # Current into battery post, A
        self.ib_in = 0.  # Current into calculate, A
        self.ib_charge = 0.  # Current into count_coulombs, A
        self.ioc = 0  # Current into battery process accounting for hysteresis, A
        self.dv_dsoc = 0.  # Slope of soc-voc curve, V/%
        self.tcharge = 0.  # Charging time to 100%, hr
        self.sr = 1  # Resistance scalar
        self.nom_vsat = bat_v_sat  # Normal battery cell saturation for SOC=99.7, V
        self.vsat = bat_v_sat  # Saturation voltage, V
        self.dvoc_dt = BATT_DVOC_DT  # Change of VOC with operating temperature in
        # range 0 - 50 C, V/deg C
        self.dt = 0  # Update time, s
        self.r_sd = r_sd
        self.tau_sd = tau_sd
        self.r0 = r0*sres
        self.tau_ct = tau_ct*stauct
        self.r_ct = float(r_ct)*sres
        self.c_ct = self.tau_ct / self.r_ct
        self.tau_dif = tau_dif*staudif
        self.r_dif = r_dif*sres
        self.c_dif = self.tau_dif / self.r_dif
        self.t_max = t_max
        self.Randles = StateSpace(2, 2, 1)
        self.temp_c = temp_c
        self.saved = Saved()  # for plots and prints
        self.dv_hys = 0.  # Placeholder so BatterySim can be plotted
        self.dv_dyn = 0.  # Placeholder so BatterySim can be plotted
        self.bms_off = False
        self.mod = 7
        self.sel = 0
        self.tweak_test = tweak_test
        self.r_ss = (r0 + r_ct + r_dif) * scale_r_ss
        self.s_hys = s_hys

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix + "Battery:\n"
        s += "  chem    = {:7.3f}  // Chemistry: 0=Battleborn, 1=CHINS\n".format(self.chem)
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
        s += "  bms_off  = {:7.1f}      // BMS off\n".format(self.bms_off)
        s += "  dv_dsoc = {:9.6f}  // Derivative scaled, V/fraction\n".format(self.dv_dsoc)
        s += "  ib =      {:7.3f}  // Battery terminal current, A\n".format(self.ib)
        s += "  vb =      {:7.3f}  // Battery terminal voltage, V\n".format(self.vb)
        s += "  voc      ={:7.3f}  // Static model open circuit voltage, V\n".format(self.voc)
        s += "  voc_stat ={:7.3f}  // Static model open circuit voltage from table (reference), V\n".format(self.voc_stat)
        s += "  vsat =    {:7.3f}  // Saturation threshold at temperature, V\n".format(self.vsat)
        s += "  dv_dyn =  {:7.3f}  // Current-induced back emf, V\n".format(self.dv_dyn)
        s += "  q =       {:7.3f}  // Present charge available to use, except q_min_, C\n".format(self.q)
        s += "  sr =      {:7.3f}  // Resistance scalar\n".format(self.sr)
        s += "  dvoc_ =   {:7.3f}  // Delta voltage, V\n".format(self.dvoc)
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
        voc = self.lut_voc.interp(soc, temp_c) + self.dvoc
        # print("soc=", soc, "temp_c=", temp_c, "dvoc=", self.dvoc, "voc=", voc)
        return voc, dv_dsoc

    def calculate(self, temp_c, soc, curr_in, dt, q_capacity, dc_dc_on, reset, rp=None, sat_init=None):
        # Battery
        raise NotImplementedError

    def init_battery(self):
        raise NotImplementedError

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
    """Extend Battery class to make a monitor"""

    def __init__(self, bat_v_sat=13.8, q_cap_rated=Battery.RATED_BATT_CAP*3600,
                 t_rated=RATED_TEMP, t_rlim=0.017, scale=1.,
                 r_sd=70., tau_sd=2.5e7, r0=0.003, tau_ct=0.2, r_ct=0.0016, tau_dif=83., r_dif=0.0077,
                 temp_c=RATED_TEMP, hys_scale=1., tweak_test=False, dv_hys=0., sres=1., stauct=1., staudif=1.,
                 scaler_q=None, scaler_r=None, scale_r_ss=1., s_hys=1., dvoc=1., eframe_mult=cp_eframe_mult,
                 scale_hys_cap=1., chem=0, coul_eff=0.9985, s_cap_chg=1., s_cap_dis=1., s_hys_chg=1., s_hys_dis=1.,
                 myCH_Tuner=1):
        q_cap_rated_scaled = q_cap_rated * scale
        Battery.__init__(self, bat_v_sat, q_cap_rated_scaled, t_rated,
                         t_rlim, r_sd, tau_sd, r0, tau_ct, r_ct, tau_dif, r_dif, temp_c, tweak_test, sres=sres,
                         stauct=stauct, staudif=staudif, scale_r_ss=scale_r_ss, s_hys=s_hys, dvoc=dvoc, chem=chem,
                         coul_eff=coul_eff)
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
            self.scaler_q = Scale(1, 4, EKF_Q_SD_NORM, EKF_Q_SD_NORM)
        else:
            self.scaler_q = scaler_q
        if scaler_r is None:
            self.scaler_r = Scale(1, 4, EKF_R_SD_NORM, EKF_R_SD_NORM)
        else:
            self.scaler_r = scaler_r
        self.Q = EKF_Q_SD_NORM * EKF_Q_SD_NORM  # EKF process uncertainty
        self.R = EKF_R_SD_NORM * EKF_R_SD_NORM  # EKF state uncertainty
        self.hys = Hysteresis(scale=hys_scale, dv_hys=dv_hys, scale_cap=scale_hys_cap, s_cap_chg=s_cap_chg,
                              s_cap_dis=s_cap_dis, s_hys_chg=s_hys_chg, s_hys_dis=s_hys_dis, chem=self.chem,
                              myCH_Tuner=myCH_Tuner)  # Battery hysteresis model - drift of voc
        self.soc_s = 0.  # Model information
        self.EKF_converged = TFDelay(False, EKF_T_CONV, EKF_T_RESET, EKF_NOM_DT)
        self.y_filt_lag = LagTustin(0.1, TAU_Y_FILT, MIN_Y_FILT, MAX_Y_FILT)
        self.WrapErrFilt = LagTustin(0.1, WRAP_ERR_FILT, -MAX_WRAP_ERR_FILT, MAX_WRAP_ERR_FILT)
        self.y_filt = 0.
        self.y_filt_2Ord = General2Pole(0.1, WN_Y_FILT, ZETA_Y_FILT, MIN_Y_FILT, MAX_Y_FILT)
        self.y_filt2 = 0.
        self.ib = 0.
        self.vb = 0.
        self.voc_stat = 0.
        self.voc_soc = 0.
        self.voc = 0.
        self.voc_filt = 0.
        self.vsat = 0.
        self.dv_dyn = 0.
        self.voc_ekf = 0.
        self.T_Rlim = RateLimit()
        self.eframe = 0
        self.eframe_mult = eframe_mult
        self.dt_eframe = self.dt*self.eframe_mult
        self.sdb_voc = SlidingDeadband(HDB_VBATT)
        self.e_wrap = 0.
        self.e_wrap_filt = 0.

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix
        s += Battery.__str__(self, prefix + 'BatteryMonitor:')
        s += "  amp_hrs_remaining_ekf_ =  {:7.3f}  // Discharge amp*time left if drain to q_ekf=0, A-h\n".format(self.amp_hrs_remaining_ekf)
        s += "  amp_hrs_remaining_wt_  =  {:7.3f}  // Discharge amp*time left if drain soc_wt_ to 0, A-h\n".format(self.amp_hrs_remaining_wt)
        s += "  q_ekf     {:7.3f}  // Filtered charge calculated by ekf, C\n".format(self.q_ekf)
        s += "  voc_soc  ={:7.3f}  // Static model open circuit voltage from table (reference), V\n".format(self.voc_soc)
        s += "  soc_ekf = {:7.3f}  // Solved state of charge, fraction\n".format(self.soc_ekf)
        s += "  tcharge = {:7.3f}  // Charging time to full, hr\n".format(self.tcharge)
        s += "  tcharge_ekf = {:7.3f}   // Charging time to full from ekf, hr\n".format(self.tcharge_ekf)
        s += "  mod     =               {:f}  // Modeling\n".format(self.mod)
        s += "  \n  "
        s += self.hys.__str__(prefix + 'BatteryMonitor:')
        s += "\n  "
        s += EKF1x1.__str__(self, prefix + 'BatteryMonitor:')
        return s

    def assign_soc_s(self, soc_s):
        self.soc_s = soc_s

    # BatteryMonitor::calculate()
    def calculate(self, chem, temp_c, vb, ib, dt, reset, updateTimeIn, q_capacity=None, dc_dc_on=None,  # BatteryMonitor
                  rp=None, u_old=None, z_old=None, bms_off_init=None):
        self.chm = chem
        if self.chm == 0:
            self.lut_voc = self.lut_voc0
        elif self.chm == 1:
            self.lut_voc = self.lut_voc1
        elif self.chm == 2:
            self.lut_voc = self.lut_voc2
        else:
            print("BatteryMonitor.calculate:  bad chem value=", chem)
            exit(1)
        self.temp_c = temp_c
        self.vsat = calc_vsat(self.temp_c)
        self.dt = dt
        self.ib_in = ib
        self.mod = rp.modeling
        self.T_Rlim.update(x=self.temp_c, reset=reset, dt=self.dt, max_=0.017, min_=-.017)
        T_rate = self.T_Rlim.rate
        self.ib = max(min(self.ib_in, IMAX_NUM), -IMAX_NUM)  # Overflow protection since ib past value used

        # Table lookup
        self.voc_soc, self.dv_dsoc = self.calc_soc_voc(self.soc, temp_c)

        # Battery management system model
        if not self.bms_off:
            voltage_low = self.voc_stat < V_BATT_DOWN
        else:
            voltage_low = self.voc_stat < V_BATT_RISING
        bms_charging = self.ib > IB_MIN_UP
        if reset and bms_off_init is not None:
            self.bms_off = bms_off_init
        else:
            self.bms_off = (self.temp_c <= low_t) or (voltage_low and not rp.tweak_test())  # KISS
        self.ib_charge = self.ib
        if self.bms_off and not bms_charging:
            self.ib_charge = 0.
        if self.bms_off and voltage_low:
            self.ib = 0.

        # Dynamic emf
        self.vb = vb
        u = np.array([self.ib, self.vb]).T
        if updateTimeIn < self.t_max:
            self.Randles.calc_x_dot(u)
            if reset or self.dt < self.t_max:
                self.Randles.update(self.dt, reset=reset)
            else:
                print("mon nan")  # freeze Randles.y
            self.voc = self.Randles.y
        else:  # aliased, unstable if update Randles
            self.voc = vb - self.r_ss * self.ib
            self.Randles.y = self.voc
        if self.bms_off and voltage_low:
            # self.voc_stat = self.voc_soc
            # self.voc = self.voc_soc
            self.voc_stat = self.vb
            self.voc = self.vb
        self.dv_dyn = self.vb - self.voc

        # Hysteresis model
        self.hys.calculate_hys(self.ib, self.soc, self.chm)
        init_low = self.bms_off or (self.soc < (self.soc_min + HYS_SOC_MIN_MARG) and self.ib > HYS_IB_THR)
        self.dv_hys = self.hys.update(self.dt, init_high=self.sat, init_low=init_low, e_wrap=self.e_wrap,
                                      chem=self.chm, scale_in=self.s_hys)
        self.voc_stat = self.voc - self.dv_hys
        self.e_wrap = self.voc_soc - self.voc_stat
        self.ioc = self.hys.ioc
        self.e_wrap_filt = self.WrapErrFilt.calculate(in_=self.e_wrap, dt=min(self.dt, F_MAX_T_WRAP), reset=reset)

        # EKF 1x1
        self.eframe_mult = max(int(float(READ_DELAY)*float(EKF_EFRAME_MULT)/1000./self.dt + 0.9999), 1)
        if self.eframe == 0:
            self.dt_eframe = self.dt * float(self.eframe_mult)
            ddq_dt = self.ib
            if ddq_dt > 0. and not self.tweak_test:
                ddq_dt *= self.coul_eff
            ddq_dt -= dqdt*self.q_capacity*T_rate
            self.Q = self.scaler_q.calculate(ddq_dt)  # TODO this doesn't work right
            self.R = self.scaler_r.calculate(ddq_dt)  # TODO this doesn't work right
            self.Q = EKF_Q_SD_NORM**2  # override
            self.R = EKF_R_SD_NORM**2  # override
            self.predict_ekf(u=ddq_dt, u_old=u_old)  # u = d(q)/dt
            self.update_ekf(z=self.voc_stat, x_min=0., x_max=1., z_old=z_old)  # z = voc, voc_filtered = hx
            self.soc_ekf = self.x_ekf  # x = Vsoc (0-1 ideal capacitor voltage) proxy for soc
            self.q_ekf = self.soc_ekf * self.q_capacity
            self.y_filt = self.y_filt_lag.calculate(in_=self.y_ekf, dt=min(self.dt_eframe, EKF_T_RESET), reset=False)
            self.y_filt2 = self.y_filt_2Ord.calculate(in_=self.y_ekf, dt=min(self.dt_eframe, TMAX_FILT), reset=False)
            # EKF convergence
            conv = abs(self.y_filt) < EKF_CONV
            self.EKF_converged.calculate(conv, EKF_T_CONV, EKF_T_RESET, min(self.dt_eframe, EKF_T_RESET), False)
            # print('dt', self.dt, 'dt_eframe', self.dt_eframe, 'eframe_mult', self.eframe_mult, 'ib', self.ib, 'temp_c', self.temp_c, 'T_rate', T_rate, 'ddq_dt', ddq_dt )
        self.eframe += 1
        if reset or self.eframe >= self.eframe_mult:  # '>=' ensures reset with changes on the fly
            self.eframe = 0

        # Filtered voc
        self.voc_filt = self.sdb_voc.update_res(self.voc, reset)

        # Charge time
        if self.ib > 0.1:
            self.tcharge_ekf = min(Battery.RATED_BATT_CAP/self.ib * (1. - self.soc_ekf), 24.)
        elif self.ib < -0.1:
            self.tcharge_ekf = max(Battery.RATED_BATT_CAP/self.ib * self.soc_ekf, -24.)
        elif self.ib >= 0.:
            self.tcharge_ekf = 24.*(1. - self.soc_ekf)
        else:
            self.tcharge_ekf = -24.*self.soc_ekf

        self.dv_dyn = self.dv_dyn
        self.voc_ekf = self.hx

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
            self.amp_hrs_remaining_ekf = amp_hrs_remaining * (self.soc_ekf - self.soc_min) / (soc - self.soc_min)
            self.amp_hrs_remaining_wt = amp_hrs_remaining * (self.soc - self.soc_min) / (soc - self.soc_min)
        else:
            self.amp_hrs_remaining_ekf = 0.
            self.amp_hrs_remaining_wt = 0.
        return self.tcharge

    # def count_coulombs(self, dt=0., reset=False, temp_c=25., charge_curr=0., sat=True):
    #     raise NotImplementedError

    def construct_state_space_monitor(self):
        """ State-space representation of dynamics
        Inputs:
            ib      Current at = vb = current at shunt, A
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
        # self.Fx = math.exp(-self.dt / self.tau_sd)
        # self.Bu = (1. - self.Fx)*self.r_sd
        self.Fx = 1. - self.dt_eframe / self.tau_sd
        self.Bu = self.dt_eframe / self.tau_sd * self.r_sd
        return self.Fx, self.Bu

    def ekf_update(self):
        # Measurement function hx(x), x = soc ideal capacitor
        x_lim = max(min(self.x_ekf, 1.), 0.)
        self.hx, self.dv_dsoc = self.calc_soc_voc(x_lim, temp_c=self.temp_c)
        # Jacobian of measurement function
        self.H = self.dv_dsoc
        return self.hx, self.H

    def init_battery(self):
        self.Randles.init_state_space([self.ib, self.vb])

    def init_soc_ekf(self, soc):
        self.soc_ekf = soc
        self.init_ekf(soc, 0.0)
        self.q_ekf = self.soc_ekf * self.q_capacity

    def regauge(self, temp_c):
        if self.converged_ekf() and abs(self.soc_ekf - self.soc) > DF2:
            print("Resetting Coulomb Counter Monitor from ", self.soc, " to EKF=", self.soc_ekf, "...")
            self.apply_soc(self.soc_ekf, temp_c)
            print("confirmed ", self.soc)

    def save(self, time, dt, soc_ref, voc_ref):  # BatteryMonitor
        self.saved.time.append(time)
        self.saved.time_min.append(time / 60.)
        self.saved.time_day.append(time / 3600. / 24.)
        self.saved.dt.append(dt)
        self.saved.chm.append(self.chm)
        self.saved.ib.append(self.ib)
        self.saved.ib_in.append(self.ib_in)
        self.saved.ib_charge.append(self.ib_charge)
        self.saved.ioc.append(self.ioc)
        self.saved.vb.append(self.vb)
        self.saved.vc.append(self.vc())
        self.saved.vd.append(self.vd())
        self.saved.dv_hys.append(self.dv_hys)
        self.saved.dv_dyn.append(self.dv_dyn)
        self.saved.voc.append(self.voc)
        self.saved.voc_soc.append(self.voc_soc)
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
        self.Randles.save(time)
        self.saved.bms_off.append(self.bms_off)
        self.saved.reset.append(self.reset)
        self.saved.e_wrap.append(self.e_wrap)
        self.saved.e_wrap_filt.append(self.e_wrap_filt)


class BatterySim(Battery):
    """Extend Battery class to make a model"""

    def __init__(self, bat_v_sat=13.8, q_cap_rated=Battery.RATED_BATT_CAP*3600,
                 t_rated=RATED_TEMP, t_rlim=0.017, scale=1.,
                 r_sd=70., tau_sd=2.5e7, r0=0.003, tau_ct=0.2, r_ct=0.0016, tau_dif=83., r_dif=0.0077, stauct=1.,
                 staudif=1., temp_c=RATED_TEMP, hys_scale=1., tweak_test=False, dv_hys=0., sres=1., scale_r_ss=1.,
                 s_hys=1., dvoc=0., scale_hys_cap=1., chem=0, coul_eff=0.9985, s_cap_chg=1., s_cap_dis=1.,
                 s_hys_chg=1., s_hys_dis=1., myCH_Tuner=1):
        Battery.__init__(self, bat_v_sat, q_cap_rated, t_rated,
                         t_rlim, r_sd, tau_sd, r0, tau_ct, r_ct, tau_dif, r_dif, temp_c, tweak_test, sres=sres,
                         stauct=stauct, staudif=staudif, scale_r_ss=scale_r_ss, s_hys=s_hys, dvoc=dvoc, chem=chem,
                         coul_eff=coul_eff)
        self.lut_voc = None
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
        self.Randles.A, self.Randles.B, self.Randles.C, self.Randles.D, self.Randles.AinvB =\
            self.construct_state_space_model()
        self.s_cap = scale  # Rated capacity scalar
        if scale is not None:
            self.apply_cap_scale(scale)
        self.hys = Hysteresis(scale=hys_scale, dv_hys=dv_hys, scale_cap=scale_hys_cap, s_cap_chg=s_cap_chg,
                              s_cap_dis=s_cap_dis, s_hys_chg=s_hys_chg, s_hys_dis=s_hys_dis, chem=self.chem,
                              myCH_Tuner=myCH_Tuner)  # Battery hysteresis model - drift of voc
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
    def calculate(self, chem, temp_c, soc, curr_in, dt, q_capacity, dc_dc_on, reset, updateTimeIn,  # BatterySim
                  rp=None, sat_init=None, bms_off_init=None):
        self.chm = chem
        if self.chm == 0:
            self.lut_voc = self.lut_voc0
        elif self.chm == 1:
            self.lut_voc = self.lut_voc1
        elif self.chm == 2:
            self.lut_voc = self.lut_voc2
        else:
            print("BatterySim.calculate:  bad chem value=", chem)
            exit(1)
        self.temp_c = temp_c
        self.dt = dt
        self.ib_in = curr_in
        if reset:
            self.ib_fut = self.ib_in
        self.ib = max(min(self.ib_fut, IMAX_NUM), -IMAX_NUM)
        self.mod = rp.modeling
        soc_lim = max(min(soc, 1.), -0.2)  # dag 9/3/2022

        # VOC-OCV model
        self.voc_stat, self.dv_dsoc = self.calc_soc_voc(soc + D_SOC_S, temp_c)
        # slightly beyond but don't windup
        self.voc_stat = min(self.voc_stat + (soc - soc_lim) * self.dv_dsoc, self.vsat * 1.2)

        # Hysteresis model
        self.hys.calculate_hys(curr_in, self.soc, self.chm)
        init_low = self.bms_off or (self.soc < (self.soc_min + HYS_SOC_MIN_MARG) and self.ib > HYS_IB_THR)
        self.dv_hys = self.hys.update(self.dt, init_high=self.sat, init_low=init_low, e_wrap=0.,
                                      chem=self.chm, scale_in=self.s_hys)
        self.voc = self.voc_stat + self.dv_hys
        self.ioc = self.hys.ioc

        # Battery management system (bms)   I believe bms can see only vb but using this for a model causes
        # lots of chatter as it shuts off, restores vb due to loss of dynamic current, then repeats shutoff.
        # Using voc_ is not better because change in dv_hys_ causes the same effect.   So using nice quiet
        # voc_stat_ for ease of simulation, not accuracy.
        if not self.bms_off:
            voltage_low = self.voc_stat < V_BATT_DOWN_SIM
        else:
            voltage_low = self.voc_stat < V_BATT_RISING_SIM
        bms_charging = self.ib_in > IB_MIN_UP
        if reset and bms_off_init is not None:
            self.bms_off = bms_off_init
        else:
            self.bms_off = (self.temp_c < low_t) or (voltage_low and not self.tweak_test)
        ib_charge_fut = self.ib_in
        if self.bms_off and not bms_charging:
            ib_charge_fut = 0.
        if self.bms_off and voltage_low:
            self.ib = 0.

        # Randles dynamic model for model, reverse version to generate sensor inputs {ib, voc} --> {vb}, ioc=ib
        u = np.array([self.ib, self.voc]).T  # past value self.ib
        self.Randles.calc_x_dot(u)
        if updateTimeIn < self.t_max:
            self.Randles.update(dt)
            if self.dt < self.t_max:
                self.vb = self.Randles.y
            else:
                self.vb = np.nan
                print("sim nan")
        else:  # aliased, unstable if update Randles
            self.vb = self.voc + self.r_ss * self.ib

        if self.bms_off:
            self.vb = self.voc
        if self.bms_off and dc_dc_on:
            self.vb = VB_DC_DC
        self.dv_dyn = self.vb - self.voc

        # Saturation logic, both full and empty
        self.vsat = self.nom_vsat + (temp_c - 25.) * self.dvoc_dt
        self.sat_ib_max = self.sat_ib_null + (1 - self.soc) * self.sat_cutback_gain * rp.cutback_gain_scalar
        if self.tweak_test or (not rp.modeling):
            self.sat_ib_max = ib_charge_fut
        self.ib_fut = min(ib_charge_fut, self.sat_ib_max)  # the feedback of self.ib
        self.ib_charge = ib_charge_fut  # same time plane as volt calcs
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

    def construct_state_space_model(self):
        """ State-space representation of dynamics
        Inputs:
            ib      Current at = vb = current at shunt, A
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
        ainvb = inv(a) * b
        return a, b, c, d, ainvb

    def count_coulombs(self, chem, dt, reset, temp_c, charge_curr, sat, soc_s_init=None, mon_delta_q=None, mon_sat=None):
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
        Outputs:
            soc     State of charge, fraction (0-1.5)
        """
        self.chm = chem
        if self.chm == 0:
            self.lut_soc_min = self.lut_soc_min0
        elif self.chm == 1:
            self.lut_soc_min = self.lut_soc_min1
        elif self.chm == 2:
            self.lut_soc_min = self.lut_soc_min2
        else:
            print("BatteryMonitor.calculate:  bad chem value=", chem)
            exit(1)
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
        self.delta_q += self.d_delta_q - dqdt*self.q_capacity*(self.temp_lim-self.t_last)
        self.delta_q = max(min(self.delta_q, 0.), -self.q_capacity*1.5)
        self.q = self.q_capacity + self.delta_q

        # Normalize
        self.soc = self.q / self.q_capacity
        self.soc_min = self.lut_soc_min.interp(self.temp_lim)
        self.q_min = self.soc_min * self.q_capacity

        # Save and return
        self.t_last = self.temp_lim
        return self.soc

    def init_battery(self):
        self.Randles.init_state_space([self.ib, self.voc])

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
        self.saved_s.bmso_s.append(self.bms_off)
        self.saved_s.Tb_s.append(self.temp_c)
        self.saved_s.Tbl_s.append(self.temp_lim)
        self.saved_s.vsat_s.append(self.vsat)
        self.saved_s.voc_s.append(self.voc)
        self.saved_s.voc_stat_s.append(self.voc_stat)
        self.saved_s.dv_dyn_s.append(self.dv_dyn)
        self.saved_s.dv_hys_s.append(self.dv_hys)
        self.saved_s.vb_s.append(self.vb)
        self.saved_s.ib_s.append(self.ib)
        self.saved_s.ib_in_s.append(self.ib_in)
        self.saved_s.ib_charge_s.append(self.ib_charge)
        self.saved_s.ib_fut_s.append(self.ib_fut)
        self.saved_s.sat_s.append(int(self.sat))
        self.saved_s.dq_s.append(self.delta_q)
        self.saved_s.soc_s.append(self.soc)
        self.saved_s.reset_s.append(self.reset)


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
        self.time_min = []
        self.time_day = []
        self.dt = []
        self.chm = []
        self.bmso = []
        self.ib = []
        self.ib_in = []
        self.ib_charge = []
        self.ioc = []
        self.vb = []
        self.vc = []
        self.vd = []
        self.voc = []
        self.voc_soc = []
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


def overall_batt(mv, sv, rv, filename,
                 mv1=None, sv1=None, rv1=None, suffix1=None, fig_files=None, plot_title=None, n_fig=None, suffix='',
                 use_time_day=False):
    if fig_files is None:
        fig_files = []

    if mv1 is None:
        if use_time_day:
            mv.time = mv.time_day - mv.time_day[0]
            sv.time = sv.time_day - sv.time_day[0]
            rv.time = rv.time_day - rv.time_day[0]

        plt.figure()  # Batt 1
        n_fig += 1
        plt.subplot(321)
        plt.title(plot_title + ' B 1')
        plt.plot(mv.time, mv.ib, color='green',   linestyle='-', label='ib'+suffix)
        plt.plot(mv.time, mv.ioc, color='magenta', linestyle='--', label='ioc'+suffix)
        plt.plot(mv.time, mv.irc, color='red', linestyle='-.', label='i_r_ct'+suffix)
        plt.plot(mv.time, mv.icd, color='cyan', linestyle=':', label='i_c_dif'+suffix)
        plt.plot(mv.time, mv.ird, color='orange', linestyle=':', label='i_r_dif'+suffix)
        plt.legend(loc=1)
        plt.subplot(323)
        plt.plot(mv.time, mv.vb, color='green', linestyle='-', label='vb'+suffix)
        plt.plot(sv.time, sv.vb, color='black', linestyle='--', label='vb_s'+suffix)
        plt.plot(mv.time, mv.vc, color='blue', linestyle='-.', label='vc'+suffix)
        plt.plot(sv.time, sv.vc, color='green', linestyle=':', label='vc_s'+suffix)
        plt.plot(mv.time, mv.vd, color='red', label='vd'+suffix)
        plt.plot(sv.time, sv.vd, color='orange', linestyle='--', label='vd_s'+suffix)
        plt.plot(mv.time, mv.voc_stat, color='orange', linestyle='-.', label='voc_stat'+suffix)
        plt.plot(sv.time, sv.voc_stat, color='cyan', linestyle=':', label='voc_stat_s,'+suffix)
        plt.plot(mv.time, mv.voc, color='magenta', label='voc'+suffix)
        plt.plot(sv.time, sv.voc, color='black', linestyle='--', label='voc_s'+suffix)
        plt.legend(loc=1)
        plt.subplot(324)
        plt.plot(mv.time, mv.vbc_dot, color='green', linestyle='-', label='vbc_dot'+suffix)
        plt.plot(mv.time, mv.vcd_dot, color='blue', linestyle='-', label='vcd_dot'+suffix)
        plt.legend(loc=1)
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
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 2
        n_fig += 1
        plt.subplot(111)
        plt.title(plot_title + ' B 2')
        plt.plot(mv.time, mv.vb, color='green', linestyle='-', label='vb'+suffix)
        plt.plot(sv.time, sv.vb, color='black', linestyle='--', label='vb_s'+suffix)
        plt.plot(mv.time, mv.vc, color='blue', linestyle='-.', label='vc'+suffix)
        plt.plot(sv.time, sv.vc, color='green', linestyle=':', label='vc_s'+suffix)
        plt.plot(mv.time, mv.vd, color='red', linestyle='-', label='vd'+suffix)
        plt.plot(sv.time, sv.vd, color='orange', linestyle='--', label='vd_s'+suffix)
        plt.plot(mv.time, mv.voc_stat, color='orange', linestyle='-.', label='voc_stat'+suffix)
        plt.plot(sv.time, sv.voc_stat, color='cyan', linestyle=':', label='voc_stat'+suffix)
        plt.plot(mv.time, mv.voc, color='magenta', linestyle='-', label='voc'+suffix)
        plt.plot(sv.time, sv.voc, color='black', linestyle='--', label='voc_s'+suffix)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 3
        n_fig += 1
        plt.subplot(321)
        plt.title(plot_title+' B 3 **SIM')
        plt.plot(sv.time, sv.ib, color='green', linestyle='-', label='ib'+suffix)
        plt.plot(sv.time, sv.irc, color='red',  linestyle='--', label='i_r_ct'+suffix)
        plt.plot(sv.time, sv.icd, color='cyan',  linestyle='-.', label='i_c_dif'+suffix)
        plt.plot(sv.time, sv.ird, color='orange', linestyle=':', label='i_r_dif'+suffix)
        plt.legend(loc=1)
        plt.subplot(323)
        plt.plot(sv.time, sv.vb, color='green', linestyle='-', label='vb'+suffix)
        plt.plot(sv.time, sv.vc, color='blue', linestyle='--', label='vc'+suffix)
        plt.plot(sv.time, sv.vd, color='red', linestyle='-.', label='vd'+suffix)
        plt.plot(sv.time, sv.voc, color='orange', label='voc'+suffix)
        plt.plot(sv.time, sv.voc_stat, color='magenta', linestyle=':', label='voc stat'+suffix)
        plt.legend(loc=1)
        plt.subplot(325)
        plt.plot(sv.time, sv.vbc_dot, color='green', linestyle='-', label='vbc_dot'+suffix)
        plt.plot(sv.time, sv.vcd_dot, color='blue', linestyle='-', label='vcd_dot'+suffix)
        plt.legend(loc=1)
        plt.subplot(322)
        plt.plot(sv.time, sv.soc, color='red', linestyle='-', label='soc'+suffix)
        plt.legend(loc=1)
        plt.subplot(326)
        plt.plot(sv.soc, sv.voc, color='black', linestyle='-', label='voc vs soc'+suffix)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 4
        n_fig += 1
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
        plt.legend(loc=1)
        plt.subplot(326)
        plt.plot(mv.time, mv.vb, color='green', linestyle='-', label='vb'+suffix)
        plt.plot(sv.time, sv.vb, color='black', linestyle='--', label='vb_s'+suffix)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 5
        n_fig += 1
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
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 6
        n_fig += 1
        plt.title(plot_title + ' B 6')
        plt.plot(mv.time, mv.e_voc_ekf, color='blue', linestyle='-.', label='e_voc'+suffix)
        plt.plot(mv.time, mv.e_soc_ekf, color='red', linestyle='dotted', label='e_soc_ekf'+suffix)
        plt.ylim(-0.01, 0.01)
        plt.legend(loc=2)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 7
        n_fig += 1
        plt.title(plot_title + ' B 7')
        plt.plot(mv.time, mv.voc, color='red', linestyle='-', label='voc'+suffix)
        plt.plot(mv.time, mv.voc_ekf, color='blue', linestyle='-.', label='voc_ekf'+suffix)
        plt.plot(sv.time, sv.voc, color='green', linestyle=':', label='voc_s'+suffix)
        plt.legend(loc=4)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 8
        n_fig += 1
        plt.title(plot_title + ' B 8')
        plt.plot(mv.time, mv.soc_ekf, color='blue', linestyle='-', label='soc_ekf'+suffix)
        plt.plot(sv.time, sv.soc, color='green', linestyle='-.', label='soc_s'+suffix)
        plt.plot(mv.time, mv.soc, color='red', linestyle=':', label='soc'+suffix)
        plt.legend(loc=4)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 9
        n_fig += 1
        plt.title(plot_title + ' B 9')
        plt.plot(mv.time, mv.e_voc_ekf, color='blue', linestyle='-.', label='e_voc'+suffix)
        plt.plot(mv.time, mv.e_soc_ekf, color='red', linestyle='dotted', label='e_soc_ekf'+suffix)
        plt.legend(loc=2)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 10
        n_fig += 1
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
        plt.legend(loc=2)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 11
        n_fig += 1
        plt.subplot(111)
        plt.title(plot_title + ' B 11')
        plt.plot(sv.soc, sv.voc_stat, color='black', linestyle='dotted', label='SIM voc_stat vs soc'+suffix)
        plt.legend(loc=2)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # Batt 12
        n_fig += 1
        plt.subplot(221)
        plt.title(plot_title + ' B 12')
        plt.plot(rv.time[1:], rv.u[1:, 1], color='red', linestyle='-', label='Mon Randles u[2]=vb'+suffix)
        plt.plot(rv.time[1:], rv.y[1:], color='blue', linestyle='-', label='Mon Randles y=voc'+suffix)
        plt.legend(loc=2)
        plt.subplot(222)
        plt.plot(rv.time[1:], rv.x[1:, 0], color='blue', linestyle='-', label='Mon Randles x[1]'+suffix)
        plt.plot(rv.time[1:], rv.x[1:, 1], color='red', linestyle='-', label='Mon Randles x[2]'+suffix)
        plt.legend(loc=2)
        plt.subplot(223)
        plt.plot(rv.time[1:], rv.x_dot[1:, 0], color='blue', linestyle='-', label='Mon Randles x_dot[1]'+suffix)
        plt.plot(rv.time[1:], rv.x_dot[1:, 1], color='red', linestyle='-', label='Mon Randles x_dot[2]'+suffix)
        plt.legend(loc=2)
        plt.subplot(224)
        plt.plot(rv.time[1:], rv.u[1:, 0], color='blue', linestyle='-', label='Mon Randles u[1]=ib'+suffix)
        plt.legend(loc=2)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

    else:
        if use_time_day:
            mv.time = mv.time_day - mv.time_day[0]
            try:
                sv.time = sv.time_day - sv.time_day[0]
            except:
                pass
            rv.time = rv.time_day - rv.time_day[0]
            mv1.time = mv1.time_day - mv1.time_day[0]
            try:
                sv1.time = sv1.time_day - sv1.time_day[0]
            except:
                pass
            rv1.time = rv1.time_day - rv1.time_day[0]
        reset_max = max(abs(min(mv.vbc_dot)), max(mv.vbc_dot), abs(min(mv1.vbc_dot)), max(mv1.vbc_dot))
        reset_index_max = max(np.where(np.array(mv1.reset) > 0))
        t_init = mv1.time[reset_index_max[-1]]
        mv.time -= t_init
        mv1.time -= t_init
        sv.time -= t_init
        sv1.time -= t_init
        rv.time -= t_init
        rv1.time -= t_init

        plt.figure()
        n_fig += 1
        plt.subplot(331)
        plt.title(plot_title + ' Battover 1')
        plt.plot(mv.time, mv.ib, color='green',   linestyle='-', label='ib'+suffix)
        plt.plot(mv1.time, mv1.ib, color='black', linestyle='--', label='ib' + suffix1)
        plt.plot(mv.time, mv.ioc, color='magenta', linestyle='-.', label='ioc'+suffix)
        plt.plot(mv1.time, mv1.ioc, color='blue', linestyle=':', label='ioc' + suffix1)
        plt.legend(loc=1)
        plt.subplot(332)
        plt.plot(mv.time, mv.irc, color='green', linestyle='-', label='i_r_ct' + suffix)
        plt.plot(mv1.time, mv1.irc, color='black', linestyle='--', label='i_r_ct' + suffix1)
        plt.legend(loc=1)
        plt.subplot(333)
        plt.plot(mv.time, mv.icd, color='green', linestyle='-', label='i_c_dif' + suffix)
        plt.plot(mv1.time, mv1.icd, color='black', linestyle='--', label='i_c_dif' + suffix1)
        plt.plot(mv.time, mv.ird, color='magenta', linestyle='-.', label='i_r_dif' + suffix)
        plt.plot(mv1.time, mv1.ird, color='blue', linestyle=':', label='i_r_dif' + suffix1)
        plt.legend(loc=1)
        plt.subplot(334)
        plt.plot(mv.time, mv.vb, color='green', linestyle='-', label='vb' + suffix)
        plt.plot(mv1.time, mv1.vb, color='black', linestyle='--', label='vb' + suffix1)
        plt.plot(mv.time, mv.vc, color='magenta', linestyle='-.', label='vc' + suffix)
        plt.plot(mv1.time, mv1.vc, color='blue', linestyle=':', label='vc' + suffix1)
        plt.legend(loc=1)
        plt.subplot(335)
        plt.plot(mv.time, mv.vd, color='green', linestyle='-', label='vd' + suffix)
        plt.plot(mv1.time, mv1.vd, color='black', linestyle='--', label='vd' + suffix1)
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
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()
        n_fig += 1
        plt.subplot(311)
        plt.title(plot_title + ' Battover 2')
        plt.plot(mv.time, mv.ib, color='green',   linestyle='-', label='ib'+suffix)
        plt.plot(mv1.time, mv1.ib, color='black', linestyle='--', label='ib' + suffix1)
        plt.plot(mv.time, mv.ioc, color='magenta', linestyle='-.', label='ioc'+suffix)
        plt.plot(mv1.time, mv1.ioc, color='blue', linestyle=':', label='ioc' + suffix1)
        plt.legend(loc=1)
        plt.subplot(312)
        plt.plot(mv.time, mv.dv_dyn, color='green', linestyle='-', label='dv_dyn'+suffix)
        plt.plot(mv1.time, mv1.dv_dyn, color='black', linestyle='--', label='dv_dyn'+suffix1)
        plt.legend(loc=1)
        plt.subplot(313)
        plt.plot(mv.time, mv.dv_hys, color='green', linestyle='-', label='dv_hys'+suffix)
        plt.plot(mv1.time, mv1.dv_hys, color='black', linestyle='--', label='dv_hys'+suffix1)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()
        n_fig += 1
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
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()
        n_fig += 1
        plt.subplot(331)
        plt.title(plot_title + ' Battover 4')
        plt.plot(rv.time[1:], rv.u[1:, 1], color='green', linestyle='-', label='Mon Randles u[2]=vb'+suffix)
        plt.plot(rv1.time[1:], rv1.u[1:, 1], color='black', linestyle='--', label='Mon Randles u[2]=vb'+suffix1)
        plt.legend(loc=2)
        plt.subplot(332)
        plt.plot(rv.time[1:], rv.y[1:], color='green', linestyle='-', label='Mon Randles y=voc'+suffix)
        plt.plot(rv1.time[1:], rv1.y[1:], color='black', linestyle='--', label='Mon Randles y=voc'+suffix1)
        plt.legend(loc=2)
        plt.subplot(333)
        plt.plot(rv.time[1:], rv.x[1:, 0], color='green', linestyle='-', label='Mon Randles x[1]'+suffix)
        plt.plot(rv1.time[1:], rv1.x[1:, 0], color='black', linestyle='--', label='Mon Randles x[1]'+suffix1)
        plt.legend(loc=2)
        plt.subplot(334)
        plt.plot(rv.time[1:], rv.x[1:, 1], color='green', linestyle='-', label='Mon Randles x[2]'+suffix)
        plt.plot(rv1.time[1:], rv1.x[1:, 1], color='black', linestyle='--', label='Mon Randles x[2]'+suffix1)
        plt.legend(loc=2)
        plt.subplot(335)
        plt.plot(rv.time[1:], rv.x_dot[1:, 0], color='green', linestyle='-', label='Mon Randles x_dot[1]'+suffix)
        plt.plot(rv1.time[1:], rv1.x_dot[1:, 0], color='black', linestyle='--', label='Mon Randles x_dot[1]'+suffix1)
        plt.legend(loc=2)
        plt.subplot(336)
        plt.plot(rv.time[1:], rv.x_dot[1:, 1], color='green', linestyle='-', label='Mon Randles x_dot[2]'+suffix)
        plt.plot(rv1.time[1:], rv1.x_dot[1:, 1], color='black', linestyle='--', label='Mon Randles x_dot[2]'+suffix1)
        plt.legend(loc=2)
        plt.subplot(337)
        plt.plot(rv.time[1:], rv.u[1:, 0], color='green', linestyle='-', label='Mon Randles u[1]=ib'+suffix)
        plt.plot(rv1.time[1:], rv1.u[1:, 0], color='black', linestyle='--', label='Mon Randles u[1]=ib'+suffix1)
        plt.legend(loc=2)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files


class SavedS:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.time_min = []
        self.time_day = []
        self.unit = []  # text title
        self.c_time = []  # Control time, s
        self.chm_s = []
        self.bmso_s = []
        self.Tb_s = []
        self.Tbl_s = []
        self.vsat_s = []
        self.voc_s = []
        self.voc_stat_s = []
        self.dv_dyn_s = []
        self.dv_hys_s = []
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
        s = "unit_m,c_time,Tb_s,Tbl_s,vsat_s,voc_stat_s,dv_dyn_s,vb_s,ib_s,sat_s,ddq_s,dq_s,q_s,qcap_s,soc_s,reset_s,\n"
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
            s += "\n"
        return s
