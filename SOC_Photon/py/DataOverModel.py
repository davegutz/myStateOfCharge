# GP_batteryEKF - general purpose battery class for EKF use
# Copyright (C) 2022 Dave Gutz
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

"""Define a general purpose battery model including Randle's model and SoC-VOV model as well as Kalman filtering
support for simplified Mathworks' tracker. See Huria, Ceraolo, Gazzarri, & Jackey, 2013 Simplified Extended Kalman
Filter Observer for SOC Estimation of Commercial Power-Oriented LFP Lithium Battery Cells.
Dependencies:
    - numpy      (everything)
    - matplotlib (plots)
    - reportlab  (figures, pdf)
"""

import numpy as np
from numpy.random import randn
import Battery
from Battery import Battery, BatteryMonitor, BatteryModel, is_sat, Retained
from Battery import overall as overalls
from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
import os
import matplotlib.pyplot as plt
from datetime import datetime
from TFDelay import TFDelay
from MonSimNomConfig import *
from MonSim import replicate

def overall(old_s, new_s, filename, fig_files=None, plot_title=None, n_fig=None, new_s_s=None):
    if fig_files is None:
        fig_files = []

    plt.figure()  # 1
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title)
    plt.plot(old_s.time, old_s.Ib, color='green', label='Ib')
    plt.plot(new_s.time, new_s.Ib, color='orange', linestyle='--', label='Ib_new')
    plt.legend(loc=1)
    plt.subplot(222)
    plt.plot(old_s.time, old_s.sat, color='black', label='sat')
    plt.plot(new_s.time, new_s.sat, color='orange', linestyle='--', label='sat_new')
    plt.plot(old_s.time, old_s.sel, color='red', label='sel')
    plt.plot(new_s.time, new_s.sel, color='blue', linestyle='--', label='sel_new')
    plt.plot(old_s.time, old_s.mod_data, color='blue', label='mod')
    plt.plot(new_s.time, new_s.mod_data, color='red', linestyle='--', label='mod_new')
    plt.legend(loc=1)
    plt.subplot(223)
    plt.plot(old_s.time, old_s.Vb, color='green', label='Vb')
    plt.plot(new_s.time, new_s.Vb, color='orange', linestyle='--', label='Vb_new')
    plt.legend(loc=1)
    plt.subplot(224)
    plt.plot(old_s.time, old_s.Voc, color='green', label='Voc')
    plt.plot(new_s.time, new_s.Voc, color='orange', linestyle='--', label='Voc_new')
    plt.plot(old_s.time, old_s.Vsat, color='blue', label='Vsat')
    plt.plot(new_s.time, new_s.Vsat, color='red', linestyle='--', label='Vsat_new')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 2
    n_fig += 1
    plt.subplot(321)
    plt.title(plot_title)
    plt.plot(old_s.time, old_s.Vdyn, color='green', label='Vdyn')
    plt.plot(new_s.time, new_s.Vdyn, color='orange', linestyle='--', label='Vdyn_new')
    plt.legend(loc=1)
    plt.subplot(322)
    plt.plot(old_s.time, old_s.Voc, color='green', label='Voc')
    plt.plot(new_s.time, new_s.Voc, color='orange', linestyle='--', label='Voc_new')
    plt.plot(old_s.time, old_s.Voc_dyn, color='blue', label='Voc_dyn')
    try:
        plt.plot(new_s.time, new_s.Voc_dyn, color='red', linestyle='--', label='Voc_dyn_new')
    except:
        plt.plot(new_s.time, new_s.voc_dyn, color='red', linestyle='--', label='Voc_dyn_new')
    plt.legend(loc=1)
    plt.subplot(323)
    plt.plot(old_s.time, old_s.Voc, color='green', label='Voc')
    plt.plot(new_s.time, new_s.Voc, color='orange', linestyle='--', label='Voc_new')
    plt.plot(old_s.time, old_s.Voc_ekf, color='blue', label='Voc_ekf')
    plt.plot(new_s.time, new_s.Voc_ekf, color='red', linestyle='--', label='Voc_ekf_new')
    plt.legend(loc=1)
    plt.subplot(324)
    plt.plot(old_s.time, old_s.y_ekf, color='green', label='y_ekf')
    plt.plot(new_s.time, new_s.y_ekf, color='orange', linestyle='--', label='y_ekf_new')
    try:
        plt.plot(new_s.time, new_s.y_filt, color='black', linestyle='--', label='y_filt_new')
        plt.plot(new_s.time, new_s.y_filt2, color='cyan', linestyle='--', label='y_filt2_new')
    except:
        print("y_filt not available pure data regression")
    plt.legend(loc=1)
    plt.subplot(325)
    plt.plot(old_s.time, old_s.dv_hys, color='green', label='dv_hys')
    plt.plot(new_s.time, new_s.dv_hys, color='orange', linestyle='--', label='dv_hys_new')
    plt.legend(loc=1)
    plt.subplot(326)
    plt.plot(old_s.time, old_s.Tb, color='green', label='temp_c')
    plt.plot(new_s.time, new_s.Tb, color='orange', linestyle='--', label='temp_c_new')
    plt.ylim(0., 50.)
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 3
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title)
    plt.plot(old_s.time, old_s.soc, color='blue', label='soc')
    plt.plot(new_s.time, new_s.soc, color='red', linestyle='--', label='soc_new')
    plt.legend(loc=1)
    plt.subplot(222)
    plt.plot(old_s.time, old_s.soc_ekf, color='green', label='soc_ekf')
    plt.plot(new_s.time, new_s.soc_ekf, color='orange', linestyle='--', label='soc_ekf_new')
    plt.legend(loc=1)
    plt.subplot(223)
    plt.plot(old_s.time, old_s.soc_wt, color='green', label='soc_wt')
    plt.plot(new_s.time, new_s.soc_wt, color='orange', linestyle='--', label='soc_wt_new')
    plt.plot(old_s.time, old_s.soc, color='blue', label='soc')
    plt.plot(new_s.time, new_s.soc, color='red', linestyle='--', label='soc_new')
    plt.legend(loc=1)
    plt.subplot(224)
    plt.plot(old_s.time, old_s.soc_m, color='green', label='soc_m')
    plt.plot(new_s.time, new_s.soc_m, color='orange', linestyle='--', label='soc_m_new')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 4
    n_fig += 1
    plt.subplot(131)
    plt.title(plot_title)
    plt.plot(old_s.time, old_s.soc, color='orange', label='soc')
    plt.plot(new_s.time, new_s.soc, color='green', linestyle='--', label='soc_new')
    if new_s_s:
        plt.plot(new_s_s.time, new_s_s.soc, color='black', linestyle='--', label='soc_m_new')
    plt.legend(loc=1)
    plt.subplot(132)
    plt.plot(old_s.time, old_s.Vb, color='orange', label='Vb')
    plt.plot(new_s.time, new_s.Vb, color='green', linestyle='--', label='Vb_new')
    if new_s_s:
        plt.plot(new_s_s.time, new_s_s.vb, color='black', linestyle='--', label='Vb_m_new')
    plt.legend(loc=1)
    plt.subplot(133)
    plt.plot(old_s.soc, old_s.Vb, color='orange', label='Vb')
    plt.plot(new_s.soc, new_s.Vb, color='green', linestyle='--', label='Vb_new')
    if new_s_s:
        plt.plot(new_s_s.soc, new_s_s.vb, color='black', linestyle='--', label='Vb_m_new')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files

def write_clean_file(txt_file, title_key, unit_key):
    csv_file = txt_file.replace('.txt', '.csv', 1)
    default_header_str = "unit,               hm,                  cTime,       dt,       sat,sel,mod,  Tb,  Vb,  Ib,        Vsat,Vdyn,Voc,Voc_ekf,     y_ekf,    soc_m,soc_ekf,soc,soc_wt,"
    # Header
    have_header_str = None
    with open(txt_file, "r") as input_file:
        with open(csv_file, "w") as output:
            for line in input_file:
                if line.__contains__(title_key):
                    if have_header_str is None:
                        have_header_str = True  # write one title only
                        output.write(line)
    if have_header_str is None:
        output.write(default_header_str)
        print("I:  using default data header")
    # Date
    with open(txt_file, "r") as input_file:
        with open(csv_file, "a") as output:
            for line in input_file:
                if line.__contains__(unit_key):
                    output.write(line)
    print("csv_file=", csv_file)
    return csv_file


class SavedData:
    def __init__(self, data=None, time_end=None):
        if data is None:
            self.i = 0
            self.time = []
            self.dt = []  # Update time, s
            self.unit = []  # text title
            self.hm = []  # hours, minutes
            self.cTime = []  # Control time, s
            self.Ib = []  # Bank current, A
            self.Vb = []  # Bank voltage, V
            self.sat = []  # Indication that battery is saturated, T=saturated
            self.sel = []  # Current source selection, 0=amp, 1=no amp
            self.mod = []  # Configuration control code, 0=all hardware, 7=all simulated, +8 tweak test
            self.Tb = []  # Battery bank temperature, deg C
            self.Vsat = []  # Monitor Bank saturation threshold at temperature, deg C
            self.Vdyn = []  # Monitor Bank current induced back emf, V
            self.dv_hys = []  # Drop across hysteresis, V
            self.Voc = []  # Monitor Static bank open circuit voltage, V
            self.Voc_dyn = []  # Bank VOC estimated from Vb and RC model, V
            self.Voc_ekf = []  # Monitor bank solved static open circuit voltage, V
            self.y_ekf = []  # Monitor single battery solver error, V
            self.soc_m = []  # Simulated state of charge, fraction
            self.soc_ekf = []  # Solved state of charge, fraction
            self.soc = []  # Coulomb Counter fraction of saturation charge (q_capacity_) availabel (0-1)
            self.soc_wt = []  # Weighted selection of ekf state of charge and Coulomb Counter (0-1)
        else:
            self.i = 0
            self.cTime = np.array(data.cTime)
            self.time = np.array(data.cTime)
            self.Ib = np.array(data.Ib)
            # manage data shape
            # Find first non-zero Ib and use to adjust time
            # Ignore initial run of non-zero Ib because resetting from previous run
            try:
                zero_start = np.where(self.Ib == 0.0)[0][0]
                self.zero_end = zero_start
                while self.Ib[self.zero_end] == 0.0:  # stop at first non-zero
                    self.zero_end += 1
            except:
                self.zero_end = 0
            time_ref = self.time[self.zero_end]
            # print("time_ref=", time_ref)
            self.time -= time_ref
            # Truncate
            if time_end is None:
                i_end = len(self.time)
            else:
                i_end = np.where(self.time <= time_end)[0][-1] + 1
            self.unit = data.unit[:i_end]
            self.hm = data.hm[:i_end]
            self.cTime = self.cTime[:i_end]
            self.dt = np.array(data.dt[:i_end])
            self.time = np.array(self.time[:i_end])
            self.Ib = np.array(data.Ib[:i_end])
            self.Vb = np.array(data.Vb[:i_end])
            self.sat = np.array(data.sat[:i_end])
            self.sel = np.array(data.sel[:i_end])
            self.mod_data = np.array(data.mod[:i_end])
            self.Tb = np.array(data.Tb[:i_end])
            self.Vsat = np.array(data.Vsat[:i_end])
            self.Vdyn = np.array(data.Vdyn[:i_end])
            self.Voc = np.array(data.Voc[:i_end])
            self.Voc_dyn = self.Vb - self.Vdyn
            self.dv_hys = self.Voc_dyn - self.Voc
            self.Voc_ekf = np.array(data.Voc_ekf[:i_end])
            self.y_ekf = np.array(data.y_ekf[:i_end])
            self.soc_m = np.array(data.soc_m[:i_end])
            self.soc_ekf = np.array(data.soc_ekf[:i_end])
            self.soc = np.array(data.soc[:i_end])
            self.soc_wt = np.array(data.soc_wt[:i_end])

    def __str__(self):
        s = "{},".format(self.unit[self.i])
        s += "{},".format(self.hm[self.i])
        s += "{:13.3f},".format(self.cTime[self.i])
        # s += "{},".format(self.T[self.i])
        s += "{:8.3f},".format(self.Ib[self.i])
        s += "{:7.2f},".format(self.Vsat[self.i])
        s += "{:5.2f},".format(self.Vdyn[self.i])
        s += "{:5.2f},".format(self.Voc[self.i])
        s += "{:5.2f},".format(self.Voc_ekf[self.i])
        s += "{:10.6f},".format(self.y_ekf[self.i])
        s += "{:7.3f},".format(self.soc_m[self.i])
        s += "{:5.3f},".format(self.soc_ekf[self.i])
        s += "{:5.3f},".format(self.soc[self.i])
        s += "{:5.3f},".format(self.soc_wt[self.i])
        return s

    def mod(self):
        return self.mod_data[self.zero_end]


if __name__ == '__main__':
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    plt.rcParams['axes.grid'] = True

    def compare_print(old_s, new_s):
        s = " time,      Ib,                   Vb,              Vdyn,          Voc,            Voc_dyn,        Voc_ekf,         y_ekf,               soc_ekf,      soc,         soc_wt,\n"
        for i in range(len(new_s.time)):
            s += "{:7.3f},".format(old_s.time[i])
            s += "{:11.3f},".format(old_s.Ib[i])
            s += "{:9.3f},".format(new_s.Ib[i])
            s += "{:9.2f},".format(old_s.Vb[i])
            s += "{:5.2f},".format(new_s.Vb[i])
            s += "{:9.2f},".format(old_s.Vdyn[i])
            s += "{:5.2f},".format(new_s.Vdyn[i])
            s += "{:9.2f},".format(old_s.Voc[i])
            s += "{:5.2f},".format(new_s.Voc[i])
            s += "{:9.2f},".format(old_s.Voc_dyn[i])
            s += "{:5.2f},".format(new_s.voc_dyn[i])
            s += "{:9.2f},".format(old_s.Voc_ekf[i])
            s += "{:5.2f},".format(new_s.Voc_ekf[i])
            s += "{:13.6f},".format(old_s.y_ekf[i])
            s += "{:9.6f},".format(new_s.y_ekf[i])
            s += "{:7.3f},".format(old_s.soc_ekf[i])
            s += "{:5.3f},".format(new_s.soc_ekf[i])
            s += "{:7.3f},".format(old_s.soc[i])
            s += "{:5.3f},".format(new_s.soc[i])
            s += "{:7.3f},".format(old_s.soc_wt[i])
            s += "{:5.3f},".format(new_s.soc_wt[i])
            s += "\n"
        return s


    def main(data_file_old_txt, unit_key):
        # Trade study inputs
        # i-->0 provides continuous anchor to reset filter (why?)  i shifts important --> 2 current sensors, hyst in ekf
        # saturation provides periodic anchor to reset filter
        # reset soc periodically anchor user display
        # tau_sd creating an anchor.   So large it's just a pass through.  TODO:  Why x correct??
        # TODO:  filter soc for saturation calculation in model
        # TODO:  temp sensitivities and mitigation

        # Config inputs
        # from MonSimNomConfig import *

        # Transient  inputs
        time_end = None
        # time_end = 2500.

        # Load data (must end in .txt)
        title_key = "unit,"  # Find one instance of title
        data_file_clean = write_clean_file(data_file_old_txt, title_key, unit_key)

        # Load
        cols = ('unit', 'hm', 'cTime', 'dt', 'sat', 'sel', 'mod', 'Tb', 'Vb', 'Ib', 'Vsat', 'Vdyn', 'Voc', 'Voc_ekf',
                'y_ekf', 'soc_m', 'soc_ekf', 'soc', 'soc_wt')
        data_old = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols, dtype=None,
                                 encoding=None).view(np.recarray)
        saved_old = SavedData(data_old, time_end)

        # Run model
        mons, sims, monrs = replicate(saved_old)

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        n_fig, fig_files = overalls(mons, sims, monrs, filename, fig_files,plot_title=plot_title, n_fig=n_fig)  # Could be confusing because sim over mon
        n_fig, fig_files = overall(saved_old, mons, filename, fig_files, plot_title=plot_title, n_fig=n_fig,
                                   new_s_s=sims)

        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='../dataReduction/figures')
        cleanup_fig_files(fig_files)

        plt.show()


    # data_file_old_txt = '../dataReduction/real world status-reflash-test 20220609.txt'; unit_key = 'soc0_2022'  # Used to filter out actual data
    # data_file_old_txt = '../dataReduction/rapidTweakRegressionTest20220613.txt'; unit_key = 'pro_2022'  # Used to filter out actual data

    # data_file_old_txt = '../dataReduction/rapidTweakRegressionTest20220613_new.txt'; unit_key = 'pro_2022'  # Used to filter out actual data
    #python DataOverModel.py("../dataReduction/rapidTweakRegressionTest20220613_new.txt", "pro_2022")
    #python DataOverModel.py("../dataReduction/2-pole y_filt, tune hys 220613.txt", "soc0_2022")

    if __name__ == "__main__":
        import sys
        print(sys.argv[1:])
        main(sys.argv[1], sys.argv[2])
