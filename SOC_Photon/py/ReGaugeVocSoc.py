# ReGaugeVocSoc:  Input measured voc_stat and original tablular t_voc_soc and produce new t_voc_soc with q=0 at soc=0
# for temp_c = Battery.RATED_TEMP
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

""" Python script that takes table input of measured voc_stat (in the form of an interim t_voc_soc and
RATED_BATT_CAPACITY/RATED_TEMP that may have soc breakpoints < 0.   Rescale and define RATED_BATT_CAPACITY that gives
q=0 at soc=0 when temp_c is RATED_TEMP."""

import numpy as np
import Battery
from Battery import overall_batt
from datetime import datetime, timedelta
from pyDAGx import myTables
from Chemistry_BMS import Chemistry


# New class with observation def added
class newChem(Chemistry):
    def __init__(self, mod_code=0, rated_batt_cap=100., scale=1.):
        Chemistry.__init__(self, mod_code=mod_code)
        self.rated_batt_cap = rated_batt_cap
        self.scale = scale

    # Assign CHINS chemistry form observation (obs)
    def assign_CH_obs(self):
        # Constants
        # self.cap = see below
        self.rated_temp = 25.  # Temperature at RATED_BATT_CAP, deg C
        self.coul_eff = 0.9976  # Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs (.9976)
        self.dqdt = 0.01  # Change of charge with temperature, fraction/deg C (0.01 from literature)
        self.dvoc_dt = 0.004  # Change of VOC with operating temperature in range 0 - 50 C V/deg C (0.004)
        self.dvoc = 0.  # Adjustment for calibration error, V (systematic error; may change in future, 0)
        self.hys_cap = 1.e4  # Capacitance of hysteresis, Farads.  tau_null = 1 / 0.001 / 1.8e4 = 0.056 s (1e4)
        self.low_voc = 9.0  # Voltage threshold for BMS to turn off battery (9.0)
        self.low_t = 0.  # Minimum temperature for valid saturation check, because BMS shuts off battery low. Heater should keep >4, too. deg C (0.)
        self.r_0 = 0.0046 * 3.  # ChargeTransfer R0, ohms  (3*0.0046)
        self.r_ct = 0.0077 * 0.76  # ChargeTransfer diffusion resistance, ohms (0.0077*0.76)
        self.r_sd = 70  # Equivalent model for EKF reference.	Parasitic discharge equivalent, ohms (70)
        self.tau_ct = 24.9  # ChargeTransfer diffusion time constant, s (=1/Rct/Cct) (24.9)
        self.tau_sd = 2.5e7  # Equivalent model for EKF reference.	Parasitic discharge time constant, sec (2.5e7)
        self.c_sd = self.tau_sd / self.r_sd
        self.vb_off = 10.  # Shutoff point in Mon, V (10.)
        self.vb_down = 9.8  # Shutoff point.  Diff to RISING needs to be larger than delta dv_hys expected, V (9.8)
        self.vb_down_sim = 9.5  # Shutoff point in Sim, V (9.5)
        self.vb_rising = 10.3  # Shutoff point when off, V (10.3)
        self.vb_rising_sim = 9.75  # Shutoff point in Sim when off, V (9.75)
        self.nom_vsat = 13.85  # Saturation threshold at temperature, deg C (13.85)
        self.r_ss = self.r_0 + self.r_ct
        self.dv_min_abs = 0.06  # Absolute value of +/- hysteresis limit, V
        self.t_dv = None

        # Tables CHINS Bmon=1, Bsim=1
        # VOC_SOC table
        t_x_soc1 = [-0.10, -0.06, -0.035, 0.00, 0.05, 0.10, 0.14, 0.17, 0.20, 0.25, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80,
                    0.90, 0.98, 0.99, 1.00]
        t_y_t1 = [5., 11.1, 11.2, 40.]
        t_voc1 = [4.00, 4.00,  4.00,  4.00,  4.00,  4.00,  10.00, 10.95, 13.03, 13.06, 13.09, 13.12, 13.17, 13.22, 13.25, 13.29, 13.30, 13.31, 13.50, 14.70,
                  4.00, 4.00,  4.00,  4.00,  4.00,  9.50,  12.97, 13.00, 13.03, 13.06, 13.09, 13.12, 13.17, 13.22, 13.25, 13.29, 13.30, 13.31, 13.50, 14.70,
                  4.00, 11.50, 12.33, 12.61, 12.81, 12.92, 12.97, 13.00, 13.03, 13.06, 13.09, 13.12, 13.17, 13.22, 13.25, 13.29, 13.30, 13.31, 13.50, 14.70,
                  4.00, 11.50, 12.33, 12.61, 12.81, 12.92, 12.97, 13.00, 13.03, 13.06, 13.09, 13.12, 13.17, 13.22, 13.25, 13.29, 13.30, 13.31, 13.50, 14.70]
        x1 = np.array(t_x_soc1)
        y1 = np.array(t_y_t1)
        data_interp1 = np.array(t_voc1)
        self.lut_voc_soc = myTables.TableInterp2D(x1, y1, data_interp1)

        # Min SOC table
        t_x_soc_min1 = t_y_t1
        t_soc_min1 = [-0.15, -0.15, -0.15, -0.15]
        self.lut_min_soc = myTables.TableInterp1D(np.array(t_x_soc_min1), np.array(t_soc_min1))

        # Hysteresis tables
        self.cap = 1e4  # scaled later
        t_soc1 = [.47, .75, .80, .86]
        self.t_dv = [-.10, -.05, -.04, 0.0, .02, .04, .05, .06, .07, .10]
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
        self.lut_r_hys = myTables.TableInterp2D(self.t_dv, t_soc1, t_r1)
        self.lut_s_hys = myTables.TableInterp2D(self.t_dv, t_soc1, t_s1)
        self.lu_x_hys = myTables.TableInterp1D(t_soc1, t_dv_max1)
        self.lu_n_hys = myTables.TableInterp1D(t_soc1, t_dv_min1)

    # Assign chemistry, anytime
    def assign_all_mod(self, mod_code=0):
        self.assign_CH_obs()

    # Regauge
    def regauge(self, new_rated_batt_cap=100., new_scale=1.):
        print('t_dv monotonic?: ', myTables.isMonotonic(self.t_dv))
        return 0

    def __str__(self, prefix=''):
        s = "new Chemistry:\n"
        s += "\n  "
        s += Chemistry.__str__(self)
        return s


# Plots
def overall():
    return 0


if __name__ == '__main__':
    import sys
    from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
    import matplotlib.pyplot as plt
    plt.rcParams['axes.grid'] = True

    def main():
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        date_ = datetime.now().strftime("%y%m%d")

        obs = newChem(mod_code=1, rated_batt_cap=100., scale=1.05)  # CHINS with RATED_BATT_CAP in local_config.h
        obs.assign_all_mod()
        obs.regauge(new_rated_batt_cap=105., new_scale=1.05)  # rescale and fix
        print('chemistry for observation', obs)  # print the result

        pathToSavePdfTo = '../dataReduction/figures'
        path_to_data = '../dataReduction'
        path_to_temp = '../dataReduction/temp'

        # Plots
        n_fig = 0
        fig_files = []
        data_root = 'ReGaugeVocSoc.py'
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        # unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo=pathToSavePdfTo)
        cleanup_fig_files(fig_files)

        plt.show()

    main()
