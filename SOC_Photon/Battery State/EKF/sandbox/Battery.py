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
        self.modeling = int(7)  # assumed for this 'model'
        self.nS = 1  # assumed for this 'model'
        self.nP = 1  # assumed for this 'model'

    def tweak_test(self):
        return ( 0x8 & self.modeling )


rp = Retained()


# Battery constants
RATED_TEMP = 25.  # Temperature at RATED_BATT_CAP, deg C
BATT_DVOC_DT = 0.004  # 5/30/2022
""" Change of VOC with operating temperature in range 0 - 50 C (0.004) V/deg C
                            >3.425 V is reliable approximation for SOC>99.7 observed in my prototype around 15-35 C"""
BATT_V_SAT = 3.4625  # Normal battery cell saturation for SOC=99.7, V (3.4625 = 13.85v)
NOM_SYS_VOLT = 12.  # Nominal system output, V, at which the reported amps are used (12)
low_voc = 10  # Minimum voltage for battery below which BMS shutsoff current
low_t = 8  # Minimum temperature for valid saturation check, because BMS shuts off battery low.
# Heater should keep >8, too
mxeps_bb = 1-1e-6  # Numerical maximum of coefficient model with scaled soc
mneps_bb = 1e-6  # Numerical minimum of coefficient model without scaled soc
DQDT = 0.01  # Change of charge with temperature, fraction/deg C.  From literature.  0.01 is commonly used
CAP_DROOP_C = 20.  # Temperature below which a floor on q arises, C (20)
TCHARGE_DISPLAY_DEADBAND = 0.1  # Inside this +/- deadband, charge time is displayed '---', A
max_voc = 1.2*NOM_SYS_VOLT  # Prevent windup of battery model, V
batt_num_cells = int(NOM_SYS_VOLT/3)  # Number of standard 3 volt LiFePO4 cells
batt_vsat = float(batt_num_cells)*BATT_V_SAT  # Total bank saturation for 0.997=soc, V
batt_vmax = (14.3/4)*float(batt_num_cells)  # Observed max voltage of 14.3 V at 25C for 12V prototype bank, V
DF1 = 0.02  # Weighted selection lower transition drift, fraction
DF2 = 0.05  # Threshold to reset Coulomb Counter if different from ekf, fraction (0.05)

class Battery(Coulombs):
    RATED_BATT_CAP = 100.
    # """Nominal battery bank capacity, Ah(100).Accounts for internal losses.This is
    #                         what gets delivered, e.g.Wshunt / NOM_SYS_VOLT.  Also varies 0.2 - 0.4 C currents
    #                         or 20 - 40 A for a 100 Ah battery"""

    # Battery model:  Randle's dynamics, SOC-VOC model

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

        # Defaults
        from pyDAGx import myTables
        t_x_soc = [0.00, 0.05, 0.10, 0.14, 0.17, 0.20, 0.25, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 0.98, 1.00]
        t_y_t = [5.,  11.1,  20.,   40.]
        t_voc = [4.00, 4.00, 6.00,  9.50,  11.70, 12.30, 12.50, 12.65, 12.82, 12.91, 12.98, 13.05, 13.11, 13.17, 13.22, 14.95,
                 4.00, 4.00, 8.00,  11.60, 12.40, 12.60, 12.70, 12.80, 12.92, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 14.96,
                 4.00, 8.00, 12.20, 12.68, 12.73, 12.79, 12.81, 12.89, 13.00, 13.04, 13.09, 13.14, 13.21, 13.25, 13.27, 15.00,
                 4.08, 8.08, 12.28, 12.76, 12.81, 12.87, 12.89, 12.97, 13.08, 13.12, 13.17, 13.22, 13.29, 13.33, 13.35, 15.08]
        x = np.array(t_x_soc)
        y = np.array(t_y_t)
        data_interp = np.array(t_voc)
        self.lut_voc = myTables.TableInterp2D(x, y, data_interp)
        self.num_cells = num_cells
        self.nz = None
        self.q = 0  # Charge, C
        self.voc = NOM_SYS_VOLT  # Model open circuit voltage, V
        self.voc_stat = self.voc  # Static model open circuit voltage from charge process, V
        self.vdyn = 0.  # Model current induced back emf, V
        self.vb = NOM_SYS_VOLT  # Battery voltage at post, V
        self.ib = 0  # Current into battery post, A
        self.ioc = 0  # Current into battery process accounting for hysteresis, A
        self.pow_oc = 0.  # Charge power, W
        self.num_cells = num_cells
        self.dv_dsoc = 0.  # Slope of soc-voc curve, V/%
        self.tcharge = 0.  # Charging time to 100%, hr
        self.sr = 1  # Resistance scalar
        self.nom_vsat = bat_v_sat * self.num_cells  # Normal battery cell saturation for SOC=99.7, V (3.4625 = 13.85v)
        self.vsat = NOM_SYS_VOLT + 1.7  # Saturation voltage, V
        self.dv = 0.01  # Adjustment for voltage level, V (0.01)
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
        # self.Randles.A, self.Randles.B, self.Randles.C, self.Randles.D = self.construct_state_space_monitor()
        self.temp_c = temp_c
        self.saved = Saved()  # for plots and prints
        self.dv_hys = 0.  # Placeholder so BatteryModel can be plotted
        self.bms_off = False
        self.mod = 7
        self.sel = 0

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix + "Battery:\n  "
        s += 'temp, #cells, dvoc_dt = {:5.1f}, {}, {:7.3f},\n'.\
            format(self.temp_c, self.num_cells, self.dvoc_dt)
        s += '  r0, r_ct, tau_ct, r_dif, tau_dif, r_sd, tau_sd = {:7.3f}, {:7.3f}, {:7.3f},' \
             ' {:7.3f}, {:7.3f}, {:7.3f}, {:7.3f},\n'.\
            format(self.r0, self.r_ct, self.tau_ct, self.r_dif, self.tau_dif, self.r_sd, self.tau_sd)
        s += "  bms_off  = {:d}      // BMS off\n".format(self.bms_off)
        s += "  dv_dsoc = {:7.3f}  // Derivative scaled, V/fraction\n".format(self.dv_dsoc)
        s += "  ib =      {:7.3f}  // Current into battery post, A\n".format(self.ib)
        s += "  vb =      {:7.3f}  // Total model voltage, voltage at terminals, V\n".format(self.vb)
        s += "  voc      ={:7.3f}  // Dynamic model open circuit voltage including hysteresis, V\n".format(self.voc)
        s += "  voc_stat ={:7.3f}  // Static model open circuit voltage, V\n".format(self.voc_stat)
        s += "  vsat =    {:7.3f}  // Saturation threshold at temperature, V\n".format(self.vsat)
        s += "  vdyn =    {:7.3f}  // Model current induced back emf, V\n".format(self.vdyn)
        s += "  q =       {:7.3f}  // Present charge, C\n".format(self.q)
        s += "  sr =      {:7.3f}  // Resistance scalar\n".format(self.sr)
        s += "  dv_ =     {:7.3f}  / Adjustment, V\n".format(self.dv)
        s += "  dt_ =     {:7.3f}  // Update time, s\n".format(self.dt)
        s += "  dv_hys  = {:7.3f}  // Hysteresis delta v, V\n".format(self.dv_hys)
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
        """SOC-OCV curve fit method per Zhang, et al """
        dv_dsoc = self.calc_h_jacobian(soc, temp_c)
        voc = self.lut_voc.interp(soc, temp_c) + self.dv
        return voc, dv_dsoc

    def calculate(self, temp_c, soc, curr_in, dt, q_capacity, dc_dc_on):
        raise NotImplementedError

    def init_battery(self):
        self.Randles.init_state_space([0., 0.])

    def look_hys(self, dv, soc):
        return NotImplementedError

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


class BatteryMonitor(Battery, EKF_1x1):
    """Extend basic class to monitor"""

    def __init__(self, t_t=None, t_b=None, t_a=None, t_c=None, m=0.478, n=0.4, d=0.707,
                 num_cells=4, bat_v_sat=3.4625, q_cap_rated=Battery.RATED_BATT_CAP*3600,
                 t_rated=RATED_TEMP, t_rlim=0.017,
                 r_sd=70., tau_sd=1.8e7, r0=0.003, tau_ct=0.2, r_ct=0.0016, tau_dif=83., r_dif=0.0077,
                 temp_c=RATED_TEMP, hys_scale=1.):
        Battery.__init__(self, t_t, t_b, t_a, t_c, m, n, d, num_cells, bat_v_sat, q_cap_rated, t_rated,
                         t_rlim, r_sd, tau_sd, r0, tau_ct, r_ct, tau_dif, r_dif, temp_c)
        self.Randles.A, self.Randles.B, self.Randles.C, self.Randles.D = self.construct_state_space_monitor()

        """ Default values from Taborelli & Onori, 2013, State of Charge Estimation Using Extended Kalman Filters for
        Battery Management System.   Battery equations from LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang
        etal.pdf.'  SOC-OCV curve fit './Battery State/BattleBorn Rev1.xls:Model Fit' using solver with min slope
        constraint >=0.02 V/soc.  m and n using Zhang values.   Had to scale soc because  actual capacity > NOM_BAT_CAP
        so equations error when soc<=0 to match data.    See Battery.h
        """
        # Parents
        EKF_1x1.__init__(self)
        self.tcharge_ekf = 0.  # Charging time to 100% from ekf, hr
        self.voc_dyn = 0.  # Charging voltage, V
        self.soc_ekf = 0.  # Filtered state of charge from ekf (0-1)
        self.SOC_ekf = 0.  # Filtered state of charge from ekf (0-100)
        self.q_ekf = 0  # Filtered charge calculated by ekf, C
        self.amp_hrs_remaining = 0  # Discharge amp*time left if drain to q=0, A-h
        self.amp_hrs_remaining_ekf = 0  # Discharge amp*time left if drain to q_ekf=0, A-h
        self.e_soc_ekf = 0.  # analysis parameter
        self.e_voc_ekf = 0.  # analysis parameter
        self.Q = 0.001*0.001  # EKF process uncertainty
        self.R = 0.1*0.1  # EKF state uncertainty
        self.hys = Hysteresis(scale=hys_scale)  # Battery hysteresis model - drift of voc
        self.soc_m = 0.  # Model information

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix + "BatteryMonitor:\n"
        s += "  voc_stat=        {:7.3f}  // Static, table lookup value of voc before applying hysteresis, V\n".\
            format(self.voc_stat)
        s += "  \n  "
        s += Battery.__str__(self, prefix + 'BatteryMonitor:')
        s += "  q_ekf     {:7.3f}  // Filtered charge calculated by ekf, C\n".format(self.q_ekf)
        s += "  tcharge = {:7.3f}  // Charging time to full, hr\n".format(self.tcharge)
        s += "  tcharge_ekf = {:7.3f}   // Charging time to full from ekf, hr\n".format(self.tcharge_ekf)
        s += "  soc_ekf = {:7.3f}  // Filtered state of charge from ekf (0-1)\n".format(self.soc_ekf)
        s += "  SOC_ekf  ={:7.3f}  // Filtered state of charge from ekf (0-100)\n".format(self.SOC_ekf)
        s += "  amp_hrs_remaining =       {:7.3f}  // Discharge amp*time left if drain to q=0, A-h\n".\
            format(self.amp_hrs_remaining,)
        s += "  amp_hrs_remaining_ekf_ =  {:7.3f}  // Discharge amp*time left if drain to q_ekf=0, A-h\n".\
            format(self.amp_hrs_remaining_ekf)
        s += "  \n  "
        s += self.hys.__str__(prefix + 'BatteryMonitor:')
        s += "\n  "
        s += EKF_1x1.__str__(self, prefix + 'BatteryMonitor:')
        return s

    def assign_soc_m(self, soc_m):
        self.soc_m = soc_m

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
        self.voc_stat = self.voc_dyn
        # Hysteresis model
        self.hys.calculate_hys(self.ib, self.voc_stat, self.soc)
        self.voc = self.hys.update(dt)
        self.ioc = self.hys.ioc
        self.dv_hys = self.hys.dv_hys
        self.pow_oc = self.voc * self.ioc
        self.bms_off = self.temp_c <= low_t;  # KISS
        if self.bms_off:
            self.voc_stat, self.dv_dsoc = self.calc_soc_voc(max(min(self.soc, 1.), 0.), temp_c)
            self.voc_stat += self.dv  # Experimentally varied
            self.ib = 0.
            self.voc = self.voc_stat
            self.vdyn = self.voc_stat
            self.voc_dyn = 0.

        # EKF 1x1
        self.predict_ekf(u=self.ib)
        self.update_ekf(z=self.voc, x_min=0., x_max=1.)
        self.soc_ekf = self.x_ekf  # x = Vsoc (0-1 ideal capacitor voltage) proxy for soc
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

        self.Ib = self.ib * rp.nP
        self.Vb = self.vb * rp.nS
        self.Voc = self.voc * rp.nS
        self.Vsat = self.vsat * rp.nS
        self.Vdyn = self.vdyn * rp.nS
        self.Voc_ekf = self.hx * rp.nS
        if rp.tweak_test():
            self.mod = 15
        else:
            self.mod = 7

        return self.soc_ekf

    def calc_charge_time(self, q, q_capacity, charge_curr, soc):
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

    # def count_coulombs(self, dt=0., reset=False, temp_c=25., charge_curr=0., sat=True, t_last=0.):
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

    def ekf_model_predict(self):
        """Process model"""
        self.Fx = math.exp(-self.dt / self.tau_sd)
        self.Bu = (1. - self.Fx)*self.r_sd
        return self.Fx, self.Bu

    def ekf_model_update(self):
        # Measurement function hx(x), x = soc ideal capacitor
        self.hx, self.dv_dsoc = self.calc_soc_voc(max(min(self.x_ekf, 1.), 0.), temp_c=self.temp_c)
        # Jacobian of measurement function
        self.H = self.dv_dsoc
        return self.hx, self.H

    def init_soc_ekf(self, soc):
        self.soc_ekf = soc
        self.init_ekf(soc, 0.0)
        self.q_ekf = self.soc_ekf * self.q_capacity
        self.SOC_ekf = self.q_ekf / self.q_cap_rated_scaled * 100.

    def save(self, time, soc_ref, voc_ref):
        self.saved.time.append(time)
        self.saved.ib.append(self.ib)
        self.saved.ioc.append(self.ioc)
        self.saved.vb.append(self.vb)
        self.saved.vc.append(self.vc())
        self.saved.vd.append(self.vd())
        self.saved.dv_hys.append(self.dv_hys)
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
        self.saved.u_ekf.append(self.u_ekf)
        self.saved.x_ekf.append(self.x_ekf)
        self.saved.y_ekf.append(self.y_ekf)
        self.saved.z_ekf.append(self.z_ekf)
        self.saved.x_prior.append(self.x_prior)
        self.saved.P_prior.append(self.P_prior)
        self.saved.x_post.append(self.x_post)
        self.saved.P_post.append(self.P_post)
        self.e_soc_ekf = (self.soc_ekf - soc_ref) / soc_ref
        self.e_voc_ekf = (self.voc_dyn - voc_ref) / voc_ref
        self.saved.e_soc_ekf.append(self.e_soc_ekf)
        self.saved.e_voc_ekf.append(self.e_voc_ekf)
        self.saved.Ib.append(self.Ib)
        self.saved.Vb.append(self.Vb)
        self.saved.Voc.append(self.Voc)
        self.saved.Vsat.append(self.Vsat)
        self.saved.Vdyn.append(self.Vdyn)
        self.saved.Voc_ekf.append(self.Voc_ekf)
        self.saved.sat.append(self.sat)
        self.saved.sel.append(self.sel)
        self.saved.mod_data.append(self.mod)
        self.saved.soc_wt.append(self.soc_wt)
        self.saved.soc_m.append(self.soc_m)

    def select(self):
        drift = self.soc_ekf - self.soc
        avg = (self.soc_ekf + self.soc) / 2.
        if drift<=-DF2 or drift>=DF2:
            self.soc_wt = self.soc_ekf
        if -DF2<drift and drift<-DF1:
            self.soc_wt = avg + (drift + DF1)/(DF2 - DF1) * (DF2/2.)
        elif DF1<drift and drift<DF2:
            self.soc_wt = avg + (drift - DF1)/(DF2 - DF1) * (DF2/2.)
        else:
            self.soc_wt = avg

class BatteryModel(Battery):
    """Extend basic class to run a model"""

    def __init__(self, t_t=None, t_b=None, t_a=None, t_c=None, m=0.478, n=0.4, d=0.707,
                 num_cells=4, bat_v_sat=3.4625, q_cap_rated=Battery.RATED_BATT_CAP * 3600,
                 t_rated=RATED_TEMP, t_rlim=0.017, scale=1.,
                 r_sd=70., tau_sd=1.8e7, r0=0.003, tau_ct=0.2, r_ct=0.0016, tau_dif=83., r_dif=0.0077,
                 temp_c=RATED_TEMP, hys_scale=1.):
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
        self.hys = Hysteresis(scale=hys_scale)  # Battery hysteresis model - drift of voc

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
        s += "  model_saturated =       {:d}  // Indicator of maximal cutback, T = cutback saturated\n".\
            format(self.model_saturated)
        s += "  ib_sat =          {:7.3f}  // Threshold to declare saturation.  This regeneratively slows" \
             " down charging so if too\n".format(self.ib_sat)
        s += "  ib     =        {:7.3f}  // Open circuit current into posts, A\n".format(self.ib)
        s += "  voc     =        {:7.3f}  // Open circuit voltage, V\n".format(self.voc)
        s += "  voc_stat=        {:7.3f}  // Static, table lookup value of voc before applying hysteresis, V\n".\
            format(self.voc_stat)
        s += "  \n  "
        s += self.hys.__str__(prefix + 'BatteryModel:')
        s += "  \n  "
        s += Battery.__str__(self, prefix + 'BatteryModel:')
        return s

    def calculate(self, temp_c, soc, curr_in, dt, q_capacity, dc_dc_on):
        self.dt = dt
        self.temp_c = temp_c
        self.bms_off = (self.temp_c < low_t) or (self.voc < low_voc)
        if self.bms_off:
            curr_in = 0.

        # soc_lim = max(min(soc, mxeps_bb), mneps_bb)
        # SOC = soc * q_capacity / self.q_cap_rated_scaled * 100.

        # VOC - OCV model
        self.voc_stat, self.dv_dsoc = self.calc_soc_voc(max(min(soc, 1.), 0.), temp_c)
        self.voc_stat += self.dv  # Experimentally varied

        # Dynamic emf
        # Hysteresis model
        self.hys.calculate_hys(curr_in, self.voc_stat, self.soc)
        self.voc = self.hys.update(self.dt)
        self.ioc = self.hys.ioc
        self.dv_hys = self.hys.dv_hys
        # Randles dynamic model for model, reverse version to generate sensor inputs {ib, voc} --> {vb}, ioc=ib
        u = np.array([self.ib, self.voc]).T  # past value self.ib
        self.Randles.calc_x_dot(u)
        self.Randles.update(dt)
        self.vb = self.Randles.y
        self.vdyn = self.vb - self.voc
        if self.bms_off and dc_dc_on:
            self.vb = 13.5
            self.vdyn = self.voc_stat
            self.voc = self.voc_stat

        # Saturation logic, both full and empty
        self.vsat = self.nom_vsat + (temp_c - 25.) * self.dvoc_dt
        self.sat_ib_max = self.sat_ib_null + (1 - self.soc) * self.sat_cutback_gain * rp.cutback_gain_scalar
        if rp.tweak_test():
            self.sat_ib_max = curr_in
            self.mod = 15
        else:
            self.mod = 7
        self.ib = min(curr_in, self.sat_ib_max)
        if ((self.q <= 0.) & (curr_in < 0.)):
            self.ib = 0.  # empty
        self.model_cutback = (self.voc_stat > self.vsat) & (self.ib == self.sat_ib_max)
        self.model_saturated = self.temp_c > low_t and \
                               ((self.voc_stat > self.vsat) & (self.ib < self.ib_sat) & (self.ib == self.sat_ib_max))
        Coulombs.sat = self.model_saturated
        self.pow_oc = self.vb * self.ib

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
        self.soc_min = self.lut_soc_min.interp(temp_lim)
        self.q_min = self.soc_min * self.q_capacity
        self.SOC = self.q / self.q_cap_rated_scaled * 100.

        # Save and return
        self.t_last = temp_lim
        return self.soc

    def save(self, time, soc_ref, voc_ref):
        self.saved.time.append(time)
        self.saved.ib.append(self.ib)
        self.saved.ioc.append(self.ioc)
        self.saved.vb.append(self.vb)
        self.saved.vc.append(self.vc())
        self.saved.vd.append(self.vd())
        self.saved.dv_hys.append(self.dv_hys)
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
        self.saved.SOC.append(self.SOC)
        self.saved.pow_oc.append(self.pow_oc)


# Other functions
def is_sat(temp_c, voc, soc):
    vsat = sat_voc(temp_c)
    return temp_c > low_t and (voc >= vsat or soc >= mxeps_bb)


def calculate_capacity(temp_c, t_sat, q_sat):
    return q_sat * (1-DQDT*(temp_c - t_sat))


def calc_vsat(temp_c):
    return sat_voc(temp_c)


def sat_voc(temp_c):
    return batt_vsat + (temp_c-25.)*BATT_DVOC_DT


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.ib = []
        self.ioc = []
        self.vb = []
        self.vc = []
        self.vd = []
        self.voc = []
        self.voc_stat = []
        self.dv_hys = []
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
        self.u_ekf = []
        self.x_ekf = []
        self.y_ekf = []
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
        self.Vdyn = []  # Monitor Bank current induced back emf, V
        self.Voc = []  # Monitor Static bank open circuit voltage, V
        self.Voc_ekf = []  # Monitor bank solved static open circuit voltage, V
        self.y_ekf = []  # Monitor single battery solver error, V
        self.soc_m = []  # Simulated state of charge, fraction
        self.soc_ekf = []  # Solved state of charge, fraction
        # self.soc = []  # Coulomb Counter fraction of saturation charge (q_capacity_) availabel (0-1)
        self.soc_wt = []  # Weighted selection of ekf state of charge and Coulomb Counter (0-1)


def overall(ms, ss, filename, fig_files=None, plot_title=None, n_fig=None):
    if fig_files is None:
        fig_files = []
    plt.figure()
    n_fig += 1
    plt.subplot(321)
    plt.title(plot_title)
    # plt.plot(ms.time, ref, color='black', label='curr dmd, A')
    plt.plot(ms.time, ms.ib, color='green', label='ib')
    plt.plot(ss.time, ss.ioc, color='magenta', label='ioc')
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
    plt.plot(ss.soc, ss.voc, color='black', linestyle='dotted', label='SIM voc vs soc')
    plt.plot(ms.soc, ms.voc, color='green', linestyle='dotted', label='MON voc vs soc')
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
    plt.plot(ss.time, ss.voc_stat, color='magenta', linestyle='dotted', label='Voc Charge')
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
    plt.plot(ms.time, ms.x_ekf, color='red', linestyle='dotted', label='x ekf')
    plt.legend(loc=4)
    plt.subplot(332)
    plt.plot(ms.time, ms.hx, color='cyan', linestyle='dotted', label='hx ekf')
    plt.plot(ms.time, ms.z_ekf, color='black', linestyle='dotted', label='z ekf')
    plt.legend(loc=4)
    plt.subplot(333)
    plt.plot(ms.time, ms.y_ekf, color='green', linestyle='dotted', label='y ekf')
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
    plt.plot(ms.time, ms.voc_dyn, color='red', linestyle='dotted', label='voc dyn est')
    plt.plot(ms.time, ms.voc, color='blue', linestyle='dotted', label='voc ekf')
    plt.plot(ss.time, ss.voc, color='green', label='voc model')
    plt.legend(loc=4)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.title(plot_title)
    plt.plot(ms.time, ms.soc_ekf, color='blue', linestyle='dotted', label='soc ekf')
    plt.plot(ss.time, ss.soc, color='green', label='soc model')
    plt.plot(ms.time, ms.soc, color='red', linestyle='dotted', label='soc counted')
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

    plt.figure()
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title)
    plt.plot(ss.time, ss.soc, color='red', label='soc')
    plt.legend(loc=1)
    plt.subplot(223)
    plt.plot(ss.time, ss.ib, color='blue', label='ib, A')
    plt.plot(ss.time, ss.ioc, color='green', label='ioc hys indicator, A')
    plt.legend(loc=1)
    plt.subplot(224)
    plt.plot(ss.time, ss.dv_hys, color='red', label='dv_hys, V')
    plt.legend(loc=2)
    fig_file_name = filename + "_" + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.subplot(111)
    plt.title(plot_title)
    plt.plot(ss.soc, ss.voc, color='black', linestyle='dotted', label='SIM voc vs soc')
    plt.legend(loc=2)
    fig_file_name = filename + "_" + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files
