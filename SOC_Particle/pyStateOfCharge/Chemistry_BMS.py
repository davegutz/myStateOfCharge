# Chemistry_BMS - battery chemical and Battery Management System (BMS) properties
# Copyright (C) 2023 Dave Gutz
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

# Constants
import numpy as np
from pyDAGx import myTables


# tentative values lags
# IB_LAG_BB = 25.*25.*3
# IB_LAG_CH = 25.*25.*3
IB_LAG_BB = 180.
IB_LAG_CH = 180.


def ib_lag(chm):
    if chm == 0.:
        lag = IB_LAG_BB
    elif chm == 1. or chm == 2.:
        lag = IB_LAG_CH
    else:
        print('bad chm value=', chm)
        return None
    return lag


class BMS:
    """Battery Management System Properties"""
    def __init__(self):
        self.low_voc = 0.  # Voltage threshold for BMS to turn off battery, V
        self.low_t = 0.  # Minimum temperature for valid saturation check, because BMS shuts off battery low.\
        # Heater should keep >4, too. deg C
        self.vb_off = 0.  # Shutoff point in Mon, V
        self.vb_down = 0.  # Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V
        self.vb_rising = 0.  # Shutoff point when off, V
        self.vb_down_sim = 0.  # Shutoff point in Sim, V
        self.vb_rising_sim = 0.  # Shutoff point in Sim when off, V


class Chemistry(BMS):
    """Properties of battery"""
    def __init__(self, mod_code=0, dvoc=0., unit=None):
        BMS.__init__(self)
        self.rated_temp = 0.  # Temperature at UNIT_CAP_RATED, deg C
        self.coul_eff = 0.  # Coulombic efficiency - the fraction of charging input that gets\
        # turned into usable Coulombs
        self.cap = 0.  # Hysteresis capacitance, F
        self.mod_code = mod_code  # Chemistry code integer
        self.dqdt = 0.  # Change of charge with temperature, fraction/deg C (0.01 from literature)
        self.m_t = 0  # Number temperature breakpoints for voc table
        self.n_s = 0  # Number of soc breakpoints voc table
        self.n_n = 0  # Number temperature breakpoints for soc_min table
        self.hys_cap = 0.  # Capacitance of hysteresis, Farads
        self.n_h = 0  # Number of dv breakpoints in r(soc, dv) table t_r, t_s
        self.m_h = 0  # Number of soc breakpoints in r(soc, dv) table t_r, t_s
        self.nom_vsat = 0.  # Saturation threshold at temperature, deg C
        self.dvoc_dt = 0.  # Change of VOC with operating temperature in range 0 - 50 C V/deg C
        self.dvoc = dvoc  # Adjustment for calibration error, V
        self.r_0 = 0.  # ChargeTransfer R0, ohms
        self.r_ct = 0.  # ChargeTransfer charge transfer resistance, ohms
        self.tau_ct = 0.  # ChargeTransfer charge transfer time constant, s (=1/Rct/Cct)
        self.tau_sd = 0.  # Equivalent model for EKF reference.	Parasitic discharge time constant, sec
        self.r_sd = 0.  # Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms
        self.c_sd = 0.  # Equivalent model for EKF reference.  Parasitic discharge equivalent, Farads
        self.r_ss = 0.  # Equivalent model for state space model initialization, ohms
        self.dv_min_abs = 0.  # Absolute value of +/- hysteresis limit, V
        self.lut_voc_soc = None
        self.lut_min_soc = None
        self.lut_r_hys = None
        self.lut_s_hys = None
        self.lu_x_hys = None
        self.lu_n_hys = None
        self.assign_all_mod(mod_code=mod_code, unit=unit)
        self.ib_lag_tau = IB_LAG_BB

    # Assign chemistry, anytime
    def assign_all_mod(self, mod_code=0, unit=None):
        if mod_code == 0:
            self.assign_BB()
        elif mod_code == 1:
            self.assign_CH(unit=unit)
        elif mod_code == 2:
            self.assign_CHG(unit=unit)

    # Assign BattleBorn chemistry
    def assign_BB(self):
        # Constants
        # self.cap = see below
        self.rated_temp = 25.  # Temperature at UNIT_CAP_RATED, deg C
        self.coul_eff = 0.9985  # Coulombic efficiency - the fraction of charging input that gets\
        # turned into usable Coulombs (0.9985)
        self.dqdt = 0.01  # Change of charge with temperature, fraction/deg C (0.01 from literature)
        self.dvoc_dt = 0.004  # Change of VOC with operating temperature in range 0 - 50 C V/deg C (0.004)
        self.dvoc = 0.  # Adjustment for calibration error, V (systematic error; may change in the future, 0)
        self.hys_cap = 3.6e3  # Capacitance of hysteresis, Farads.  // div 10 6/13/2022 to match data.\
        # // div 10 again 9/29/2022 // div 10 again 11/30/2022 (3.6e3) # tau_null = 1 / 0.005 / 3.6e3 = 0.056 s
        self.low_voc = 9.0  # Voltage threshold for BMS to turn off battery, V (9.0)
        self.low_t = 0  # Minimum temperature for valid saturation check, because BMS shuts off battery low.\
        # Heater should keep >4, too. deg C (0)
        self.r_0 = 0.0113  # ChargeTransfer R0, ohms (0.0113)
        self.r_ct = 0.001  # ChargeTransfer diffusion resistance, ohms (0.001)
        self.r_sd = 70  # Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms (70.)
        self.tau_ct = 83.  # ChargeTransfer diffusion time constant, s (=1/Rct/Cct) (83.)
        self.tau_sd = 2.5e7  # Equivalent model for EKF reference.	Parasitic discharge time constant, sec (1.87e7)
        self.c_sd = self.tau_sd / self.r_sd
        self.vb_off = 10.  # Shutoff point in Mon, V (10.)
        self.vb_down = 9.8  # Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V (9.8)
        self.vb_down_sim = 9.5  # Shutoff point in Sim, V (9.5)
        self.vb_rising = 10.3  # Shutoff point when off, V (10.3)
        self.vb_rising_sim = 9.75  # Shutoff point in Sim when off, V (9.75)
        self.nom_vsat = 13.85 - 0.05  # Saturation threshold at temperature, deg C (13.85 - 0.05 HDB_VB)
        self.r_ss = self.r_0 + self.r_ct
        self.dv_min_abs = 0.3  # Absolute value of +/- hysteresis limit, V

        # Tables Battleborn Bmon=0, Bsim=0
        # VOC_SOC table
        t_x_soc0 = [-0.15, 0.00, 0.05, 0.10, 0.14, 0.17, 0.20, 0.25, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 0.99,
                    0.995, 1.00]
        t_y_t0 = [5., 11.1, 20., 30., 40.]
        t_voc0 = [4.00, 4.00, 4.00,  4.00,  10.50, 12.00, 12.75, 13.00, 13.07, 13.20, 13.21, 13.28, 13.35, 13.41, 13.47, 13.52, 13.69, 14.25,
                  4.00, 4.00, 4.00,  9.80,  12.30, 12.80, 13.00, 13.10, 13.20, 13.26, 13.31, 13.36, 13.41, 13.47, 13.50, 13.53, 13.70, 14.26,
                  4.00, 4.00, 10.30, 12.90, 13.07, 13.15, 13.19, 13.25, 13.29, 13.33, 13.34, 13.39, 13.44, 13.51, 13.55, 13.57, 13.82, 14.30,
                  4.00, 4.00, 12.30, 12.95, 13.05, 13.10, 13.15, 13.25, 13.30, 13.38, 13.42, 13.46, 13.50, 13.54, 13.56, 13.57, 13.82, 14.30,
                  4.00, 4.00, 12.30, 12.95, 13.05, 13.10, 13.15, 13.25, 13.30, 13.38, 13.42, 13.46, 13.50, 13.54, 13.56, 13.57, 13.82, 14.30]

        x0 = np.array(t_x_soc0)
        y0 = np.array(t_y_t0)
        data_interp0 = np.array(t_voc0)
        self.lut_voc_soc = myTables.TableInterp2D(x0, y0, data_interp0)

        # Min SOC table
        t_x_soc_min0 = [5., 11.1, 20., 30., 40.]
        t_soc_min0 = [0.07, 0.05, -0.05, 0.00, 0.20]
        self.lut_min_soc = myTables.TableInterp1D(np.array(t_x_soc_min0), np.array(t_soc_min0))

        # Hysteresis tables
        self.cap = 3.6e3
        t_dv0 = [-0.7,   -0.5,  -0.3,  0.0,   0.15,   0.3,   0.7]
        t_soc0 = [0, .5, .7]
        t_s0 = [1., 1., 1., 1., 1., 1., 1.,
                1., 1., 1., 1., 1., 1., 1.,
                1., 1., 1., 1., 1., 1., 1.]
        t_dv_min0 = [-0.7, -0.5, -0.3]
        t_dv_max0 = [0.7,  0.3,  0.2]
        t_r0 = [0.019, 0.015, 0.016, 0.009, 0.011, 0.017, 0.030,
                0.014, 0.014, 0.010, 0.008, 0.010, 0.015, 0.015,
                0.016, 0.016, 0.013, 0.005, 0.007, 0.010, 0.010]
        self.lut_r_hys = myTables.TableInterp2D(t_dv0, t_soc0, t_r0)
        self.lut_s_hys = myTables.TableInterp2D(t_dv0, t_soc0, t_s0)
        self.lu_x_hys = myTables.TableInterp1D(t_soc0, t_dv_max0)
        self.lu_n_hys = myTables.TableInterp1D(t_soc0, t_dv_min0)

    # Assign CHINS chemistry
    def assign_CH(self, unit=None):
        # Constants
        # self.cap = see below
        self.rated_temp = 25.  # Temperature at UNIT_CAP_RATED, deg C
        self.coul_eff = 0.9976  # Coulombic efficiency - the fraction of charging input that gets turned into\
        # usable Coulombs (.9976)
        self.dqdt = 0.01  # Change of charge with temperature, fraction/deg C (0.01 from literature)
        self.dvoc_dt = -.01  # Change of VOC with operating temperature in range 0 - 50 C V/deg C (0.004)
        self.dvoc = 0.  # Adjustment for calibration error, V (systematic error; may change in the future, 0)
        self.hys_cap = 1.e4  # Capacitance of hysteresis, Farads.  tau_null = 1 / 0.001 / 1.8e4 = 0.056 s (1e4)
        self.low_voc = 9.0  # Voltage threshold for BMS to turn off battery (9.0)
        self.low_t = 0.  # Minimum temperature for valid saturation check, because BMS shuts off battery low.\
        # Heater should keep >4, too. deg C (0.)
        self.r_0 = 0.0046*3.  # ChargeTransfer R0, ohms  (3*0.0046)
        self.r_ct = 0.0077*0.76  # ChargeTransfer diffusion resistance, ohms (0.0077*0.76)
        self.r_sd = 70  # Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms (70)
        self.tau_ct = 24.9  # ChargeTransfer diffusion time constant, s (=1/Rct/Cct) (24.9)
        self.tau_sd = 2.5e7  # Equivalent model for EKF reference.	Parasitic discharge time constant, sec (2.5e7)
        self.c_sd = self.tau_sd / self.r_sd
        self.vb_off = 11.  # Shutoff point in Mon, V (11.)
        self.vb_down = 10.6  # Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V (10.6)
        self.vb_down_sim = 10.5  # Shutoff point in Sim, V (10.5)
        self.vb_rising = 11.3  # Shutoff point when off, V (11.3)
        self.vb_rising_sim = 10.75  # Shutoff point in Sim when off, V (10.75)
        self.nom_vsat = 13.85 - 0.05  # Saturation threshold at temperature, deg C (13.85 - 0.05 HDB_VB)
        self.r_ss = self.r_0 + self.r_ct
        self.dv_min_abs = 0.06  # Absolute value of +/- hysteresis limit, V
        self.ib_lag_tau = IB_LAG_CH  # Lag time to wash out sat effect on dv, s

        # Tables CHINS Bmon=1, Bsim=1, from ReGaugeVocSoc 3/2/2023 soc0p
        # VOC_SOC table
        # Tables CHINS Bmon=1, Bsim=1, from CompareTensorData 8/31/2023
        t_x_soc1 = [-0.035, 0.000, 0.050, 0.100, 0.108, 0.120, 0.140, 0.170, 0.200, 0.250, 0.300, 0.3400, 0.400,
                    0.500, 0.600, 0.700, 0.800, 0.900, 0.980, 0.990, 1.000]
        t_y_t1 = [0.0, 11.0, 21.5]
        t_voc1 = [4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 9.000, 11.770, 12.700,
                  12.950, 13.050, 13.100, 13.226, 13.259, 13.264, 13.460, 14.270,
                  4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 9.000, 11.770, 12.700,
                  12.950, 13.050, 13.100, 13.226, 13.259, 13.264, 13.460, 14.270,
                  4.000, 4.000, 9.0000, 9.500, 11.260, 11.850, 12.400, 12.650, 12.730, 12.810, 12.920, 12.960,
                  13.020, 13.060, 13.220, 13.280, 13.284, 13.299, 13.310, 13.486, 14.700]
        t_x_soc_min1 = [0.000, 11.00, 21.5, 40.000]
        t_soc_min1 = [0.31, 0.31, 0.1, 0.1]

        # Form tables
        x1 = np.array(t_x_soc1)
        y1 = np.array(t_y_t1)
        data_interp1 = np.array(t_voc1)
        self.lut_voc_soc = myTables.TableInterp2D(x1, y1, data_interp1)
        self.lut_min_soc = myTables.TableInterp1D(np.array(t_x_soc_min1), np.array(t_soc_min1))

        # Hysteresis tables
        self.cap = 1e4  # scaled later
        t_soc1 = [.47, .75, .80, .86]
        t_dv1 = [-.10, -.05, -.04, 0.0, .02, .04, .05, .06, .07, .10]
        schp4 = [0.003, 0.003, 0.4, 0.4, 0.4, 0.4, 0.010, 0.010, 0.010, 0.010]
        schp8 = [0.004, 0.004, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.014, 0.012]
        schp9 = [0.004, 0.004, 0.4, 0.4, .2, .09, 0.04, 0.006, 0.006, 0.006]
        t_r1 = schp4 + schp8 + schp8 + schp9
        t_dv_min1 = [-0.06, -0.06, -0.06, -0.06]
        t_dv_max1 = [0.06, 0.1, 0.1, 0.06]
        SRs1p4 = [1., 1., .2, .2, .2, .2, 1., 1., 1., 1.]
        SRs1p8 = [1., 1., .2, .2, .2, 1., 1., 1., 1., 1.]
        SRs1p9 = [1., 1., .1, .1, .2, 1., 1., 1., 1., 1.]
        t_s1 = SRs1p4 + SRs1p8 + SRs1p8 + SRs1p9
        self.lut_r_hys = myTables.TableInterp2D(t_dv1, t_soc1, t_r1)
        self.lut_s_hys = myTables.TableInterp2D(t_dv1, t_soc1, t_s1)
        self.lu_x_hys = myTables.TableInterp1D(t_soc1, t_dv_max1)
        self.lu_n_hys = myTables.TableInterp1D(t_soc1, t_dv_min1)

    # Assign CHINS chemistry
    def assign_CHG(self, unit=None):
        # Constants
        # self.cap = see below
        self.rated_temp = 25.  # Temperature at UNIT_CAP_RATED, deg C
        self.coul_eff = 0.9976  # Coulombic efficiency - the fraction of charging input that gets turned into\
        # usable Coulombs (.9976)
        self.dqdt = 0.01  # Change of charge with temperature, fraction/deg C (0.01 from literature)
        self.dvoc_dt = -.01  # Change of VOC with operating temperature in range 0 - 50 C V/deg C (0.004)
        self.dvoc = -0.14  # Adjustment for calibration error, V (systematic error; may change in the future, 0)
        self.hys_cap = 1.e4  # Capacitance of hysteresis, Farads.  tau_null = 1 / 0.001 / 1.8e4 = 0.056 s (1e4)
        self.low_voc = 9.0  # Voltage threshold for BMS to turn off battery (9.0)
        self.low_t = 0.  # Minimum temperature for valid saturation check, because BMS shuts off battery low.\
        # Heater should keep >4, too. deg C (0.)
        self.r_0 = 0.0046*3.  # ChargeTransfer R0, ohms  (3*0.0046)
        self.r_ct = 0.0077*0.76  # ChargeTransfer diffusion resistance, ohms (0.0077*0.76)
        self.r_sd = 70  # Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms (70)
        self.tau_ct = 24.9  # ChargeTransfer diffusion time constant, s (=1/Rct/Cct) (24.9)
        self.tau_sd = 2.5e7  # Equivalent model for EKF reference.	Parasitic discharge time constant, sec (2.5e7)
        self.c_sd = self.tau_sd / self.r_sd
        self.vb_off = 11.  # Shutoff point in Mon, V (11.)
        self.vb_down = 10.6  # Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V (10.6)
        self.vb_down_sim = 10.5  # Shutoff point in Sim, V (10.5)
        self.vb_rising = 11.3  # Shutoff point when off, V (11.3)
        self.vb_rising_sim = 10.75  # Shutoff point in Sim when off, V (10.75)
        self.nom_vsat = 13.85 - 0.05  # Saturation threshold at temperature, deg C (13.85 - 0.05 HDB_VB)
        self.r_ss = self.r_0 + self.r_ct
        self.dv_min_abs = 0.06  # Absolute value of +/- hysteresis limit, V
        self.ib_lag_tau = IB_LAG_CH  # Lag time to wash out sat effect on dv, s

        # 2024-04-24T14-51-24:  tune to data
        t_y_t1 = [21.5, 25.0, 35.0, ]
        t_x_soc1 = [-0.400, -0.300, -0.230, -0.200, -0.150, -0.130, -0.114, -0.044, 0.000, 0.016, 0.032, 0.055,
                    0.064, 0.114, 0.134, 0.154, 0.183, 0.214, 0.300, 0.400, 0.500, 0.600, 0.700, 0.800, 0.900,
                    0.960, 0.980, 1.000, ]
        t_voc1 = [
            4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 8.170,
            11.285, 12.114, 12.558, 12.707, 12.875, 13.002, 13.054, 13.201, 13.275, 13.284, 13.299, 13.307, 13.310,
            14.700,
            4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 4.000, 7.947, 11.000, 11.946, 12.252, 12.588, 12.670, 12.797,
            12.833, 12.864, 12.908, 12.957, 13.034, 13.081, 13.106, 13.159, 13.234, 13.272, 13.286, 13.300, 13.300,
            14.760,
            4.000, 4.000, 6.686, 8.206, 10.739, 12.045, 12.411, 12.799, 12.866, 12.890, 12.914, 12.949, 12.963,
            13.037, 13.052, 13.067, 13.089, 13.112, 13.146, 13.196, 13.284, 13.318, 13.320, 13.320, 13.320, 13.320,
            13.320, 14.760,
        ]
        t_x_soc_min1 = [21.5, 25.0, 35.0, ]
        t_soc_min1 = [0.13, 0.00, -0.14, ]

        # Form tables
        x1 = np.array(t_x_soc1)
        y1 = np.array(t_y_t1)
        data_interp1 = np.array(t_voc1)
        self.lut_voc_soc = myTables.TableInterp2D(x1, y1, data_interp1)
        self.lut_min_soc = myTables.TableInterp1D(np.array(t_x_soc_min1), np.array(t_soc_min1))

        # Hysteresis tables
        self.cap = 1e4  # scaled later
        t_soc1 = [.47, .75, .80, .86]
        t_dv1 = [-.10, -.05, -.04, 0.0, .02, .04, .05, .06, .07, .10]
        schp4 = [0.003, 0.003, 0.4, 0.4, 0.4, 0.4, 0.010, 0.010, 0.010, 0.010]
        schp8 = [0.004, 0.004, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.014, 0.012]
        schp9 = [0.004, 0.004, 0.4, 0.4, .2, .09, 0.04, 0.006, 0.006, 0.006]
        t_r1 = schp4 + schp8 + schp8 + schp9
        t_dv_min1 = [-0.06, -0.06, -0.06, -0.06]
        t_dv_max1 = [0.06, 0.1, 0.1, 0.06]
        SRs1p4 = [1., 1., .2, .2, .2, .2, 1., 1., 1., 1.]
        SRs1p8 = [1., 1., .2, .2, .2, 1., 1., 1., 1., 1.]
        SRs1p9 = [1., 1., .1, .1, .2, 1., 1., 1., 1., 1.]
        t_s1 = SRs1p4 + SRs1p8 + SRs1p8 + SRs1p9
        self.lut_r_hys = myTables.TableInterp2D(t_dv1, t_soc1, t_r1)
        self.lut_s_hys = myTables.TableInterp2D(t_dv1, t_soc1, t_s1)
        self.lu_x_hys = myTables.TableInterp1D(t_soc1, t_dv_max1)
        self.lu_n_hys = myTables.TableInterp1D(t_soc1, t_dv_min1)

    def lookup_voc(self, soc_, temp_c_):
        return self.lut_voc_soc.interp(soc_, temp_c_) + self.dvoc

    def __str__(self, prefix=''):
        s = prefix + "Chemistry:\n"
        s += "  mod_code =     {:2.0f}      // Chemistry code integer\n".format(self.mod_code)
        s += "  low_voc   =    {:7.3f}  // Voltage threshold for BMS to turn off battery, V\n".format(self.low_voc)
        s += "  low_t     =    {:7.3f}  // Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C\n".format(self.low_t)
        s += "  vb_off    =    {:7.3f}  // Shutoff point in Mon, V\n".format(self.vb_off)
        s += "  vb_down   =    {:7.3f}  // Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V\n".format(self.vb_down)
        s += "  vb_rising =    {:7.3f}  // Shutoff point when off, V\n".format(self.vb_rising)
        s += "  vb_down_sim=   {:7.3f}  // Shutoff point in Sim, V\n".format(self.vb_down_sim)
        s += "  vb_rising_sim ={:7.3f}  // Shutoff point in Sim when off, V\n".format(self.vb_rising_sim)
        s += "  rated_temp =   {:7.3f}  // Temperature at UNIT_CAP_RATED, deg C\n".format(self.rated_temp)
        s += "  coul_eff =      {:6.4f}  // Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs\n".format(self.coul_eff)
        s += "  cap      =  {:10.1f}  // Hysteresis capacitance, Farads\n".format(self.cap)
        s += "  dqdt      =    {:7.3f}  // Change of charge with temperature, fraction/deg C (0.01 from literature)\n".format(self.dqdt)
        s += "  dv_min_abs=    {:7.3f}  // Absolute value of +/- hysteresis limit, V\n".format(self.dv_min_abs)
        s += "  m_t =          {:2.0f}      // Number temperature breakpoints for voc table\n".format(self.m_t)
        s += "  n_s =          {:2.0f}      // Number of soc breakpoints voc table\n".format(self.n_s)
        s += "  n_n =          {:2.0f}      // Number temperature breakpoints for soc_min table\n".format(self.n_n)
        s += "  n_h =          {:2.0f}      // Number of dv breakpoints in r(soc, dv) table t_r, t_s\n".format(self.n_h)
        s += "  m_h =          {:2.0f}      // Number of soc breakpoints in r(soc, dv) table t_r, t_s\n".format(self.m_h)
        s += "  hys_cap =      {:7.0f}      // Capacitance of hysteresis, Farads\n".format(self.hys_cap)
        s += "  nom_vsat =      {:7.3f}  // Saturation threshold at temperature, deg C\n".format(self.nom_vsat)
        s += "  dvoc_dt =       {:7.3f}  // Change of VOC with operating temperature in range 0 - 50 C V/deg C\n".format(self.dvoc_dt)
        s += "  dvoc =          {:7.3f}  // Adjustment for calibration error, V\n".format(self.dvoc)
        s += "  c_sd =        {:9.3f}  // Equivalent model for EKF reference.  Parasitic discharge equivalent, Farads\n".format(self.c_sd)
        s += "  r_0 =         {:9.3f}  // ChargeTransfer R0, ohms\n".format(self.r_0)
        s += "  r_ct =        {:9.3f}  // ChargeTransfer charge transfer resistance, ohms\n".format(self.r_ct)
        s += "  tau_ct =      {:9.3f}  // ChargeTransfer charge transfer time constant, s (=1/Rct/Cct)\n".format(self.tau_ct)
        s += "  r_sd =        {:9.3f}  // Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms\n".format(self.r_sd)
        s += "  tau_sd =      {:9.3f}  // Equivalent model for EKF reference.	Parasitic discharge time constant, sec\n".format(self.tau_sd)
        s += "  r_ss =        {:9.3f}  // Equivalent model for state space model initialization, ohms\n".format(self.r_ss)
        s += "  \n  {}\n".format(self.lut_voc_soc.__str__('voc(t, soc)'), '  ')
        s += "  {}\n".format(self.lut_min_soc.__str__('soc_min(temp_c)', '  '))
        s += "  {}\n".format(self.lut_r_hys.__str__('res(dv_hys)', '  '))
        s += "  {}\n".format(self.lut_s_hys.__str__('slr(dv_hys)', '  '))
        s += "  {}\n".format(self.lu_x_hys.__str__('dv_max(soc)', '  '))
        s += "  {}\n".format(self.lu_n_hys.__str__('dv_min(soc)', '  '))
        return s
