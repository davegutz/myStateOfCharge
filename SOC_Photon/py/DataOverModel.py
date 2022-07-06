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
import matplotlib.pyplot as plt
from datetime import datetime
from MonSim import replicate, save_clean_file, save_clean_file_sim
from kivy.utils import platform
if platform != 'linux':
    from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files

def overall(old_s, new_s, old_s_sim, new_s_sim, new_s_sim_m, filename, fig_files=None, plot_title=None, n_fig=None, new_s_s=None):
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
    plt.plot(old_s.time, old_s.Voc_stat, color='green', label='Voc_stat')
    plt.plot(new_s.time, new_s.Voc_stat, color='orange', linestyle='--', label='Voc_stat_new')
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
    plt.plot(old_s.time, old_s.dV_dyn, color='green', label='dV_dyn')
    plt.plot(new_s.time, new_s.dv_dyn, color='orange', linestyle='--', label='dv_dyn_new')
    plt.legend(loc=1)
    plt.subplot(322)
    plt.plot(old_s.time, old_s.Voc_stat, color='green', label='Voc_stat')
    plt.plot(new_s.time, new_s.Voc_stat, color='orange', linestyle='--', label='Voc_stat_new')
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
    plt.plot(old_s.time, old_s.dV_hys, color='green', label='dV_hys')
    if new_s_sim:
        tt = np.array(old_s.time)
        Vbt = np.array(old_s.Vb)
        from pyDAGx import myTables
        lut_vb = myTables.TableInterp1D(tt, Vbt)
        n = len(new_s_sim.time)
        voc_req = np.zeros((n,1))
        dv_hys_req = np.zeros((n,1))
        voc_stat_req = np.zeros((n,1))
        for i in range(n):
            voc_req[i] = lut_vb.interp(new_s_sim.time[i]) - new_s_sim.dv_dyn[i]
            voc_stat_req[i] = new_s_sim.voc_stat[i]
            dv_hys_req[i] = voc_req[i] - new_s_sim.voc_stat[i]
        plt.plot(new_s_sim.time, new_s_sim.dv_hys, color='red', linestyle='--', label='dv_hys_m_new')
        plt.plot(new_s_sim.time, dv_hys_req, color='black', linestyle='--', label='dv_hys_req_m_new')

    if old_s_sim:
        tt = np.array(old_s.time)
        Vbt = np.array(old_s.Vb)
        from pyDAGx import myTables
        lut_vb = myTables.TableInterp1D(tt, Vbt)
        n = len(old_s_sim.time)
        voc_req = np.zeros((n,1))
        dv_hys_req = np.zeros((n,1))
        voc_stat_req = np.zeros((n,1))
        for i in range(n):
            voc_req[i] = lut_vb.interp(old_s_sim.time[i]) - old_s_sim.dv_dyn_m[i]
            voc_stat_req[i] = old_s_sim.voc_stat_m[i]
            dv_hys_req[i] = voc_req[i] - old_s_sim.voc_stat_m[i]
        plt.plot(old_s_sim.time, old_s_sim.dv_hys_m, color='black', label='dv_hys_m')
        plt.plot(old_s_sim.time, dv_hys_req, color='orange', linestyle='--', label='dv_hys_req_m')
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
    plt.plot(old_s.time, old_s.soc, color='blue', label='soc')
    plt.plot(new_s.time, new_s.soc, color='red', linestyle='--', label='soc_new')
    plt.plot(old_s.time, old_s.soc_m, color='green', label='soc_m')
    plt.plot(new_s.time, new_s.soc_m, color='orange', linestyle='--', label='soc_m_new')
    plt.plot(old_s.time, old_s.soc_ekf, color='cyan', label='soc_ekf')
    plt.plot(new_s.time, new_s.soc_ekf, color='black', linestyle='--', label='soc_ekf_new')
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
    plt.plot(new_s.time, new_s.soc_ekf, color='cyan', linestyle='--', label='soc_ekf_new')
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

    if old_s_sim and new_s_sim_m:
        plt.figure()  # sim_m  1
        n_fig += 1
        plt.subplot(331)
        plt.title(plot_title)
        plt.plot(old_s_sim.time, old_s_sim.ib_m, color='orange', label='ib_m')
        plt.plot(new_s_sim_m.time, new_s_sim_m.ib_m, color='green', linestyle='--', label='ib_m_new')
        plt.plot(old_s.time, old_s.Ib, color='blue', label='ib')
        plt.plot(new_s.time, new_s.Ib, color='red', linestyle='--', label='ib_new')
        plt.legend(loc=1)
        plt.subplot(332)
        plt.plot(old_s_sim.time, old_s_sim.soc_m, color='orange', label='soc_m')
        plt.plot(new_s_sim_m.time, new_s_sim_m.soc_m, color='green', linestyle='--', label='soc_m_new')
        plt.legend(loc=1)
        plt.subplot(333)
        plt.plot(old_s_sim.time, old_s_sim.voc_stat_m, color='orange', label='voc_stat_m')
        plt.plot(new_s_sim_m.time, new_s_sim_m.voc_stat_m, color='green', linestyle='--', label='voc_stat_m_new')
        plt.plot(old_s_sim.time, old_s_sim.vsat_m, color='blue', label='vsat_m')
        plt.plot(new_s_sim_m.time, new_s_sim_m.vsat_m, color='red', linestyle='--', label='vsat_m_new')
        plt.plot(old_s_sim.time, old_s_sim.vb_m, color='cyan', label='vb_m')
        plt.plot(new_s_sim_m.time, new_s_sim_m.vb_m, color='black', linestyle='--', label='vb_m_new')
        plt.legend(loc=1)
        plt.subplot(334)
        plt.plot(old_s_sim.time, old_s_sim.Tb_m, color='orange', label='Tb_m')
        plt.plot(new_s_sim_m.time, new_s_sim_m.Tb_m, color='green', linestyle='--', label='Tb_m_new')
        plt.plot(old_s_sim.time, old_s_sim.Tbl_m, color='blue', label='Tbl_m')
        plt.plot(new_s_sim_m.time, new_s_sim_m.Tbl_m, color='red', linestyle='--', label='Tbl_m_new')
        plt.legend(loc=1)
        plt.subplot(335)
        plt.plot(old_s_sim.time, old_s_sim.dv_dyn_m, color='orange', label='dv_dyn_m')
        plt.plot(new_s_sim_m.time, new_s_sim_m.dv_dyn_m, color='green', linestyle='--', label='dv_dyn_m_new')
        plt.legend(loc=1)
        plt.subplot(337)
        plt.plot(old_s_sim.time, old_s_sim.dq_m, color='orange', label='dq_m')
        plt.plot(new_s_sim_m.time, new_s_sim_m.dq_m, color='green', linestyle='--', label='dq_m_new')
        plt.legend(loc=1)
        plt.subplot(338)
        plt.plot(old_s_sim.time, old_s_sim.reset_m, color='orange', label='reset_m')
        plt.plot(new_s_sim_m.time, new_s_sim_m.reset_m, color='red', linestyle='--', label='reset_m_new')
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # sim_m  2
        n_fig += 1
        plt.subplot(331)
        plt.title(plot_title)
        plt.plot(old_s.time, old_s.Vb, color='red', label='Vb')
        plt.plot(old_s.time, old_s.Voc, color='blue', label='Voc')
        plt.plot(old_s.time, old_s.Voc_stat, color='green', label='Voc_stat')
        plt.legend(loc=1)
        plt.subplot(332)
        plt.plot(old_s_sim.time, old_s_sim.vb_m, color='red', label='vb_m')
        plt.plot(old_s_sim.time, old_s_sim.voc_m, color='blue', label='voc_m')
        plt.plot(old_s_sim.time, old_s_sim.voc_stat_m, color='green', label='voc_stat_m')
        plt.legend(loc=1)
        plt.subplot(334)
        plt.plot(new_s.time, new_s.vb, color='red', label='vb_new')
        plt.plot(new_s.time, new_s.voc, color='blue', label='voc_new')
        plt.plot(new_s.time, new_s.voc_stat, color='green', label='voc_stat_new')
        plt.legend(loc=1)
        plt.subplot(335)
        plt.plot(new_s_sim_m.time, new_s_sim_m.vb_m, color='red', label='vb_m_new')
        plt.plot(new_s_sim_m.time, new_s_sim_m.voc_m, color='blue', label='voc_m_new')
        plt.plot(new_s_sim_m.time, new_s_sim_m.voc_stat_m, color='green', label='voc_stat_m_new')
        plt.legend(loc=1)
        plt.subplot(337)
        plt.plot(old_s.time, old_s.soc, color='orange', label='soc')
        plt.plot(old_s.time, old_s.soc_ekf, color='blue', label='soc_ekf')
        plt.plot(new_s.time, new_s.soc, color='cyan', linestyle='--', label='soc_new')
        plt.plot(new_s.time, new_s.soc_ekf, color='red', linestyle='--', label='soc_ekf_new')
        plt.legend(loc=1)
        plt.subplot(338)
        plt.plot(old_s_sim.time, old_s_sim.soc_m, color='orange', label='soc_m')
        plt.plot(new_s_sim_m.time, new_s_sim_m.soc_m, color='cyan', linestyle='--', label='soc_m_new')
        plt.legend(loc=1)
        plt.subplot(333)
        plt.title(plot_title)
        plt.plot(old_s.time, old_s.Vb, color='red', label='Vb')
        plt.plot(old_s_sim.time, old_s_sim.vb_m, color='blue', label='vb_m')
        plt.plot(new_s.time, new_s.vb, color='magenta', label='vb_new')
        plt.plot(new_s_sim_m.time, new_s_sim_m.vb_m, color='green', label='vb_m_new')
        plt.legend(loc=1)
        plt.subplot(336)
        plt.title(plot_title)
        plt.plot(old_s.time, old_s.Voc, color='red', label='Voc')
        plt.plot(new_s.time, new_s.voc, color='magenta', label='voc_new')
        plt.plot(old_s_sim.time, old_s_sim.voc_m, color='blue', label='voc_m')
        plt.plot(new_s_sim_m.time, new_s_sim_m.voc_m, color='green', label='voc_m_new')
        plt.legend(loc=1)
        plt.subplot(339)
        plt.title(plot_title)
        plt.plot(old_s.time, old_s.Voc_stat, color='red', label='Voc_stat')
        plt.plot(old_s_sim.time, old_s_sim.voc_stat_m, color='blue', label='voc_stat_m')
        plt.plot(new_s.time, new_s.voc_stat, color='magenta', label='voc_stat_new')
        plt.plot(new_s_sim_m.time, new_s_sim_m.voc_stat_m, color='green', label='voc_stat_m_new')
        plt.legend(loc=1)

        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files

def write_clean_file(txt_file, type, title_key, unit_key):
    csv_file = txt_file.replace('.txt', type + '.csv', 1)
    # default_header_str = "unit,               hm,                  cTime,       dt,       sat,sel,mod,  Tb,  Vb,  Ib,        Vsat,dV_dyn,Voc_stat,Voc_ekf,     y_ekf,    soc_m,soc_ekf,soc,soc_wt,"
    # Header
    have_header_str = None
    with open(txt_file, "r") as input_file:
        with open(csv_file, "w") as output:
            for line in input_file:
                if line.__contains__(title_key):
                    if have_header_str is None:
                        have_header_str = True  # write one title only
                        output.write(line)
    # if have_header_str is None:
    #     with open(csv_file, "w") as output:
    #         output.write(default_header_str)
    #         print("I:  using default data header")
    # Data
    num_lines = 0
    with open(txt_file, "r") as input_file:
        with open(csv_file, "a") as output:
            for line in input_file:
                if line.__contains__(unit_key):
                    output.write(line)
                    num_lines += 1
    if not num_lines:
        csv_file = None
        print("I(write_clean_file): no data to write")
    else:
        print("Wrote(write_clean_file):", csv_file)
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
            self.dV_dyn = []  # Monitor Bank current induced back emf, V
            self.dv_hys = []  # Drop across hysteresis, V
            self.Voc_stat = []  # Monitor Static bank open circuit voltage, V
            self.Voc = []  # Bank VOC estimated from Vb and RC model, V
            self.Voc_ekf = []  # Monitor bank solved static open circuit voltage, V
            self.y_ekf = []  # Monitor single battery solver error, V
            self.soc_m = []  # Simulated state of charge, fraction
            self.soc_ekf = []  # Solved state of charge, fraction
            self.soc = []  # Coulomb Counter fraction of saturation charge (q_capacity_) availabel (0-1)
            self.soc_wt = []  # Weighted selection of ekf state of charge and Coulomb Counter (0-1)
            self.time_ref = 0.  # Adjust time for start of Ib input
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
            self.time_ref = self.time[self.zero_end]
            # print("time_ref=", self.time_ref)
            self.time -= self.time_ref
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
            self.dV_dyn = np.array(data.dV_dyn[:i_end])
            self.Voc_stat = np.array(data.Voc_stat[:i_end])
            self.Voc = self.Vb - self.dV_dyn
            self.dV_hys = self.Voc - self.Voc_stat
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
        s += "{:5.2f},".format(self.dV_dyn[self.i])
        s += "{:5.2f},".format(self.Voc_stat[self.i])
        s += "{:5.2f},".format(self.Voc_ekf[self.i])
        s += "{:10.6f},".format(self.y_ekf[self.i])
        s += "{:7.3f},".format(self.soc_m[self.i])
        s += "{:5.3f},".format(self.soc_ekf[self.i])
        s += "{:5.3f},".format(self.soc[self.i])
        s += "{:5.3f},".format(self.soc_wt[self.i])
        return s

    def mod(self):
        return self.mod_data[self.zero_end]


class SavedDataSim:
    def __init__(self, time_ref, data=None, time_end=None):
        if data is None:
            self.i = 0
            self.time = []
            self.unit = []  # text title
            self.c_time = []  # Control time, s
            self.Tb_m = []
            self.Tbl_m = []
            self.vsat_m = []
            self.voc_m = []
            self.voc_stat_m = []
            self.dv_dyn_m = []
            self.dv_hys_m = []
            self.vb_m = []
            self.ib_m = []
            self.sat_m = []
            self.ddq_m = []
            self.dq_m = []
            self.q_m = []
            self.qcap_m = []
            self.soc_m = []
            self.reset_m = []
        else:
            self.i = 0
            self.c_time = np.array(data.c_time)
            self.time = np.array(data.c_time) - time_ref
            # Truncate
            if time_end is None:
                i_end = len(self.time)
            else:
                i_end = np.where(self.time <= time_end)[0][-1] + 1
            self.unit_m = data.unit_m[:i_end]
            self.c_time = self.c_time[:i_end]
            self.time = self.time[:i_end]
            self.Tb_m = data.Tb_m[:i_end]
            self.Tbl_m = data.Tbl_m[:i_end]
            self.vb_m = data.vb_m[:i_end]
            self.vsat_m = data.vsat_m[:i_end]
            self.voc_stat_m = data.voc_stat_m[:i_end]
            self.dv_dyn_m = data.dv_dyn_m[:i_end]
            self.voc_m = self.vb_m - self.dv_dyn_m
            self.dv_hys_m = self.voc_m - self.voc_stat_m
            self.ib_m = data.ib_m[:i_end]
            self.sat_m = data.sat_m[:i_end]
            self.ddq_m = data.ddq_m[:i_end]
            self.dq_m = data.dq_m[:i_end]
            self.q_m = data.q_m[:i_end]
            self.qcap_m = data.qcap_m[:i_end]
            self.soc_m = data.soc_m[:i_end]
            self.reset_m = data.reset_m[:i_end]

    def __str__(self):
        s = "{},".format(self.unit[self.i])
        s += "{:13.3f},".format(self.c_time[self.i])
        s += "{:5.2f},".format(self.Tb_m[self.i])
        s += "{:5.2f},".format(self.Tbl_m[self.i])
        s += "{:8.3f},".format(self.vsat_m[self.i])
        s += "{:5.2f},".format(self.voc_stat_m[self.i])
        s += "{:5.2f},".format(self.dv_dyn_m[self.i])
        s += "{:5.2f},".format(self.vb_m[self.i])
        s += "{:8.3f},".format(self.ib_m[self.i])
        s += "{:7.3f},".format(self.sat_m[self.i])
        s += "{:5.3f},".format(self.ddq_m[self.i])
        s += "{:5.3f},".format(self.dq_m[self.i])
        s += "{:5.3f},".format(self.qcap_m[self.i])
        s += "{:7.3f},".format(self.soc_m[self.i])
        s += "{:d},".format(self.reset_m[self.i])
        return s

    def mod(self):
        return self.mod_data[self.zero_end]


if __name__ == '__main__':
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    plt.rcParams['axes.grid'] = True

    def compare_print(old_s, new_s):
        s = " time,      Ib,                   Vb,              dV_dyn,          Voc_stat,            Voc,        Voc_ekf,         y_ekf,               soc_ekf,      soc,         soc_wt,\n"
        for i in range(len(new_s.time)):
            s += "{:7.3f},".format(old_s.time[i])
            s += "{:11.3f},".format(old_s.Ib[i])
            s += "{:9.3f},".format(new_s.Ib[i])
            s += "{:9.2f},".format(old_s.Vb[i])
            s += "{:5.2f},".format(new_s.Vb[i])
            s += "{:9.2f},".format(old_s.dV_dyn[i])
            s += "{:5.2f},".format(new_s.dV_dyn[i])
            s += "{:9.2f},".format(old_s.Voc_stat[i])
            s += "{:5.2f},".format(new_s.Voc_stat[i])
            s += "{:9.2f},".format(old_s.Voc[i])
            s += "{:5.2f},".format(new_s.voc[i])
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

        # Load data (must end in .txt) txt_file, type, title_key, unit_key
        data_file_clean = write_clean_file(data_file_old_txt, type='_mon', title_key='unit,', unit_key=unit_key)
        data_file_sim_clean = write_clean_file(data_file_old_txt, type='_sim', title_key='unit_m', unit_key='unit_sim,')

        # Load
        cols = ('unit', 'hm', 'cTime', 'dt', 'sat', 'sel', 'mod', 'Tb', 'Vb', 'Ib', 'Vsat', 'dV_dyn', 'Voc_stat', 'Voc_ekf',
                'y_ekf', 'soc_m', 'soc_ekf', 'soc', 'soc_wt')
        data_old = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols, dtype=None,
                                 encoding=None).view(np.recarray)
        saved_old = SavedData(data_old, time_end)
        cols_sim = ('unit_m', 'c_time', 'Tb_m', 'Tbl_m', 'vsat_m', 'voc_stat_m', 'dv_dyn_m', 'vb_m', 'ib_m', 'sat_m', 'ddq_m',
                    'dq_m', 'q_m', 'qcap_m', 'soc_m', 'reset_m')
        try:
            data_old_sim = np.genfromtxt(data_file_sim_clean, delimiter=',', names=True, usecols=cols_sim, dtype=None,
                                 encoding=None).view(np.recarray)
            saved_old_sim = SavedDataSim(saved_old.time_ref, data_old_sim, time_end)
        except:
            data_old_sim = None
            saved_old_sim = None

        # Run model
        mons, sims, monrs, sims_m = replicate(saved_old)
        date_ = datetime.now().strftime("%y%m%d")
        mon_file_save = data_file_clean.replace(".csv", "_rep.csv")
        save_clean_file(mons, mon_file_save, '_mon_rep' + date_)
        if data_file_sim_clean:
            sim_file_save = data_file_sim_clean.replace(".csv", "_rep.csv")
            save_clean_file_sim(sims_m, sim_file_save, '_sim_rep' + date_)

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        data_root = data_file_clean.split('/')[-1].replace('.csv', '-')
        if platform == 'linux':
            filename = "Python/Figures/" + data_root + sys.argv[0].split('/')[-1]
        else:
            filename = "./Figures/" + data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        # n_fig, fig_files = overalls(mons, sims, monrs, filename, fig_files,plot_title=plot_title, n_fig=n_fig)  # Could be confusing because sim over mon
        n_fig, fig_files = overall(saved_old, mons, saved_old_sim, sims, sims_m, filename, fig_files, plot_title=plot_title,
                                   n_fig=n_fig, new_s_s=sims)
        if platform != 'linux':
            unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='../dataReduction/figures')
            cleanup_fig_files(fig_files)

        plt.show()


    #python DataOverModel.py("../dataReduction/rapidTweakRegressionTest20220613_new.txt", "pro_2022")
    #python DataOverModel.py("../dataReduction/2-pole y_filt, tune hys 220613.txt", "soc0_2022")
    #python DataOverModel.py("../dataReduction/watchXm2.txt", "pro_2022")
    #python DataOverModel.py("../dataReduction/serial_20220624_095543.txt", "pro_2022")
    #python DataOverModel.py("../dataReduction/rapidTweakRegressionTest20220626.txt", "pro_2022")
    #python DataOverModel.py("../dataReduction/slowTweakRegressionTest20220626.txt", "pro_2022")
    #
    """
    PyCharm Sample Run Configuration Parameters:
    "../dataReduction/serial_20220624_095543.txt"
    pro_2022
    
    PyCharm Terminal:
    python DataOverModel.py "../dataReduction/serial_20220624_095543.txt" "pro_2022"

    android:
    python Python/DataOverModel.py "USBTerminal/serial_20220624_095543.txt" "pro_2022"
    """

    if __name__ == "__main__":
        import sys
        print(sys.argv[1:])
        main(sys.argv[1], sys.argv[2])
