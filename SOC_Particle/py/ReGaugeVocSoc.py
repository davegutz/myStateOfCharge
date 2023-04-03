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
from datetime import datetime
from pyDAGx import myTables
from Chemistry_BMS import Chemistry


def pretty_print_vec(vec, prefix='', spacer=''):
    s = prefix + "(vector):\n"
    s += spacer + prefix + " = ["
    count = 1
    N = len(vec)
    for X in vec:
        s += " {:6.3f}".format(X)
        if count < N:
            s += ","
            count += 1
    s += "]\n"
    print(s)


# New class with observation def added
class localChem(Chemistry):
    def __init__(self, mod_code=0, rated_batt_cap=100., scale=1., keep_sim_happy=0.5):
        Chemistry.__init__(self, mod_code=mod_code)
        self.rated_batt_cap = rated_batt_cap
        self.scale = scale
        self.new_rated_batt_cap = None
        self.new_scale = None
        self.t_dv = None
        self.t_dv_max = None
        self.t_dv_min = None
        self.s_off_obs = None
        self.t_r = None
        self.t_s = None
        self.t_soc = None
        self.t_soc_max = None
        self.t_soc_min = None
        self.t_voc = None
        self.t_x_soc = None
        self.t_x_soc_min = None
        self.t_y_t = None
        self.t_voc_new = None
        self.lut_voc_soc_new = None
        self.t_soc_min_new = None
        self.lut_min_soc_new = None
        self.keep_sim_happy = None  # Schedule -0.5 v so that sim shuts off lower than monitor because sim stops running when q=0

    # Assign CHINS chemistry form observation (obs)
    # Hardcoded in here so can change Chemistry object with result
    def assign_obs(self, keep_sim_happy=0.5):
        # Constants
        # self.cap = see below
        self.rated_temp = 25.  # Temperature at UNIT_CAP_RATED, deg C
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

        # Tables CHINS Bmon=1, Bsim=1
        # VOC_SOC table
        t_x_soc1 = [-0.10, -0.06, -0.035, 0.00, 0.05, 0.10, 0.14, 0.17, 0.20, 0.25, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80,
                    0.90, 0.98, 0.99, 1.00]
        t_y_t1 = [5., 11.1, 11.2, 40.]
        t_voc1 = [4.00, 4.00,  4.00,  4.00,  4.00,  4.00,  10.00, 10.95, 13.03, 13.06, 13.09, 13.12, 13.17, 13.22, 13.25, 13.29, 13.30, 13.31, 13.50, 14.70,
                  4.00, 4.00,  4.00,  4.00,  4.00,  9.50,  12.97, 13.00, 13.03, 13.06, 13.09, 13.12, 13.17, 13.22, 13.25, 13.29, 13.30, 13.31, 13.50, 14.70,
                  4.00, 11.50, 12.33, 12.61, 12.81, 12.92, 12.97, 13.00, 13.03, 13.06, 13.09, 13.12, 13.17, 13.22, 13.25, 13.29, 13.30, 13.31, 13.50, 14.70,
                  4.00, 11.50, 12.33, 12.61, 12.81, 12.92, 12.97, 13.00, 13.03, 13.06, 13.09, 13.12, 13.17, 13.22, 13.25, 13.29, 13.30, 13.31, 13.50, 14.70]
        self.t_x_soc = np.array(t_x_soc1)
        self.t_y_t = np.array(t_y_t1)
        self.t_voc = np.array(t_voc1)
        self.lut_voc_soc = myTables.TableInterp2D(self.t_x_soc, self.t_y_t, self.t_voc)
        self.keep_sim_happy = keep_sim_happy
        self.s_off_obs = self.lut_voc_soc.r_interp(self.vb_down_sim-self.keep_sim_happy, self.rated_temp)
        print('s_off_obs', self.s_off_obs)

        # Min SOC table
        self.t_x_soc_min = self.t_y_t.copy()
        t_soc_min1 = [-0.15, -0.15, -0.15, -0.15]
        self.t_soc_min = np.array(t_soc_min1)
        self.lut_min_soc = myTables.TableInterp1D(self.t_x_soc_min, self.t_soc_min)

        # Hysteresis tables
        self.cap = 1e4  # scaled later
        self.t_soc = [.47, .75, .80, .86]
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
        self.t_s = SRs1p4 + SRs1p8 + SRs1p8 + SRs1p9
        self.t_r = np.array(t_r1)
        self.t_dv_max = np.array(t_dv_max1)
        self.t_dv_min = np.array(t_dv_min1)
        self.lut_r_hys = myTables.TableInterp2D(self.t_dv, self.t_soc, self.t_r)
        self.lut_s_hys = myTables.TableInterp2D(self.t_dv, self.t_soc, self.t_s)
        self.lu_x_hys = myTables.TableInterp1D(self.t_soc, self.t_dv_max)
        self.lu_n_hys = myTables.TableInterp1D(self.t_soc, self.t_dv_min)

    # Assign chemistry, anytime
    def assign_all_mod(self, mod_code=0):
        self.assign_obs()

    # Regauge
    # Assuming flat voc(soc), may simply scale soc as a practical matter
    def regauge(self):
        print('t_dv monotonic?: ', myTables.isMonotonic(self.t_dv))
        temp_c = self.rated_temp
        print('  x    v')
        for v in np.arange(0., 15., 0.1):
            x = self.lut_voc_soc.r_interp(v, temp_c, verbose=False)
            print("{:8.4f}".format(x), "{:8.4f}".format(v))
        scale = 1. - self.s_off_obs
        pretty_print_vec(self.t_x_soc, prefix='t_x_soc', spacer='  ')
        print(' soc   soc_scale v_old   v_new')
        t_voc_new = []
        for temp_c in self.t_y_t:
            for soc in self.t_x_soc:
                soc_scale = 1. - (1. - soc)*scale
                v_old = self.lut_voc_soc.interp(soc, temp_c)
                v_new = self.lut_voc_soc.interp(soc_scale, temp_c)
                t_voc_new.append(v_new)
                print("{:6.3f}".format(soc), "{:6.3f}".format(soc_scale), "{:6.2f}".format(v_old), "{:6.2f}".format(v_new))
        self.t_voc_new = np.array(t_voc_new)
        self.lut_voc_soc_new = myTables.TableInterp2D(self.t_x_soc, self.t_y_t, t_voc_new)
        self.new_rated_batt_cap = self.scale*self.rated_batt_cap*(1.-self.s_off_obs)
        # self.new_scale = self.scale - self.s_off_obs
        self.new_scale = self.new_rated_batt_cap / self.rated_batt_cap
        t_soc_min_new = []
        for temp_c in self.t_y_t:
            t_soc_min_new.append(self.lut_voc_soc_new.r_interp(self.vb_off, temp_c))
        self.t_soc_min_new = np.array(t_soc_min_new)
        self.lut_min_soc_new = myTables.TableInterp1D(self.t_x_soc_min, self.t_soc_min_new)

    def __str__(self, prefix=''):
        s = "new Chemistry:\n"
        s += "\n  "
        s += Chemistry.__str__(self)
        s += "\nObs lut_voc_soc:\n"
        s += self.lut_voc_soc.__str__()
        s += "\nNew lut_voc_soc:\n"
        s += self.lut_voc_soc_new.__str__()
        s += "\nNew lut_soc_min:\n"
        s += self.lut_min_soc_new.__str__()
        s += "\nIn app use \n#define UNIT_CAP_RATED        {:5.1f}   // Nominal battery unit capacity at RATED_TEMP.  (* 'Sc' or '*BS'/'*BP'), Ah\n".\
            format(self.new_rated_batt_cap)
        s += "In Python use UNIT_CAP_RATED " + "{:7.2f}".format(self.rated_batt_cap)
        s += "  and allow ucrs over UART to automatically set simulation values (in replicate())"

        return s

    def plot(self, filename='', fig_files=None, plot_title=None, n_fig=None):
        N = len(self.t_x_soc)
        M = len(self.t_y_t)
        plt.figure()  # GP 1
        n_fig += 1
        plt.subplot(111)
        plt.title(plot_title + ' New Schedule')
        for j in range(M):
            t_voc_x = self.t_voc[j*N:(j+1)*N]
            plt.plot(self.t_x_soc, t_voc_x, color='black', linestyle='-')
        for j in range(M):
            t_voc_x = self.t_voc_new[j*N:(j+1)*N]
            plt.plot(self.t_x_soc, t_voc_x, color='red', linestyle='--')
        # plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        return n_fig, fig_files


# Plots
def overall():
    return 0


if __name__ == '__main__':
    import sys
    from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
    import matplotlib.pyplot as plt
    plt.rcParams['axes.grid'] = True

    #  Instructions:
    #  Copy current values of obs battery from Chemistry_BMS for mod_code entered below in localChem, into assign_obs above
    #  Check values below in localChem() for mod_code, rated_batt_cap, scale
    #    mod_code agrees with entries (for proper plotting and book-keeping)
    #    rated_batt_cap from Battery.UNIT_CAP_RATED in Python, not #define in app
    #    scale from scale_in in CompareRunSim.   If doesn't appear in CompareRunSim there is a default value of 1.
    def main():
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        date_ = datetime.now().strftime("%y%m%d")

        obs = localChem(mod_code=1, rated_batt_cap=100., scale=1.05)  # rated_batt_cap/scale in Python
        obs.assign_all_mod()
        obs.regauge()  # rescale and fix
        print('chemistry for observation', obs)  # print the observation

        # Plots
        n_fig = 0
        fig_files = []
        data_root = 'ReGaugeVocSoc.py'
        pathToSavePdfTo = '../dataReduction/figures'
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        n_fig, fig_files = obs.plot(filename, fig_files=fig_files, plot_title=plot_title, n_fig=n_fig)
        precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=pathToSavePdfTo)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo=pathToSavePdfTo)
        cleanup_fig_files(fig_files)

        plt.show()

    main()
