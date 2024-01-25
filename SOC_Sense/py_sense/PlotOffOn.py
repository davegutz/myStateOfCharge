# PlotSimS - general purpose plotting, Off / On related
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

"""Define a general purpose battery model including Randles' model and SoC-VOV model as well as Kalman filtering
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
from Battery import overall_batt
# below suppresses runtime error display******************
# import os
# os.environ["KIVY_NO_CONSOLELOG"] = "1"
# from kivy.utils import platform  # failed experiment to run BLE data plotting realtime on android
# if platform != 'linux':
#     from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
plt.rcParams.update({'figure.max_open_warning': 0})


def off_on_plot(mo, mv, so, sv, smv, filename, fig_files=None, plot_title=None, fig_list=None, plot_init_in=False,
                ref_str='_ref', test_str='_test'):
    if so and smv:
        fig_list.append(plt.figure())  # 7 off/on sim
        plt.subplot(221)
        plt.title(plot_title + ' off/on sim 1')
        plt.plot(so.time, so.vb_s, color='black', linestyle='-', label='vb_s' + ref_str)
        if hasattr(sv, 'vb'):
            plt.plot(sv.time, sv.vb, color='orange', linestyle=':', label='vb_s' + test_str)
        if hasattr(sv, 'vb_s'):
            plt.plot(sv.time, sv.vb_s, color='orange', linestyle=':', label='vb_s' + test_str)
        plt.plot(so.time, so.voc_s, color='blue', linestyle='--', label='voc_s' + ref_str)
        if hasattr(sv, 'voc'):
            plt.plot(sv.time, sv.voc, color='red', linestyle='--', label='voc_s' + test_str)
        elif hasattr(sv, 'voc_s'):
            plt.plot(sv.time, sv.voc_s, color='red', linestyle='--', label='voc_s' + test_str)
            plt.plot(so.time, so.voc_stat_s, color='magenta', linestyle='-.', label='voc_stat_s' + ref_str)
        if hasattr(sv, 'voc_stat'):
            plt.plot(sv.time, sv.voc_stat, color='green', linestyle=':', label='voc_stat_s' + test_str)
        elif hasattr(sv, 'voc_stat_s'):
            plt.plot(sv.time, sv.voc_stat_s, color='green', linestyle=':', label='voc_stat_s' + test_str)
        plt.legend(loc=1)
        plt.subplot(222)
        plt.plot(so.time, so.dv_hys_s, linestyle='-', color='black', label='dv_hys_s' + ref_str)
        if hasattr(sv, 'dv_hys'):
            plt.plot(sv.time, sv.dv_hys, linestyle='--', color='orange', label='dv_hys_s' + test_str)
        elif hasattr(sv, 'dv_hys_s'):
            plt.plot(sv.time, sv.dv_hys_s, linestyle='--', color='orange', label='dv_hys_s' + test_str)
        plt.legend(loc=1)
        plt.subplot(223)
        plt.plot(so.time, so.soc_s, linestyle='-', color='black', label='soc_s' + ref_str)
        if hasattr(sv, 'soc'):
            plt.plot(sv.time, sv.soc, linestyle='--', color='orange', label='soc_s' + test_str)
        elif hasattr(sv, 'soc_s'):
            plt.plot(sv.time, sv.soc_s, linestyle='--', color='orange', label='soc_s' + test_str)
        plt.legend(loc=1)
        plt.subplot(224)
        plt.plot(so.time, so.ib_s, linestyle='-', color='black', label='ib_s' + ref_str)
        if hasattr(sv, 'ib'):
            plt.plot(sv.time, sv.ib, linestyle='--', color='orange', label='ib_s' + test_str)
        if hasattr(sv, 'ib_s'):
            plt.plot(sv.time, sv.ib_s, linestyle='--', color='orange', label='ib_s' + test_str)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        fig_list.append(plt.figure())  # 8 off/on mon
        plt.subplot(221)
        plt.title(plot_title + ' off/on mon 1')
        plt.plot(mo.time, mo.vb, color='black', linestyle='-', label='vb' + ref_str)
        plt.plot(mo.time, mo.voc, color='blue', linestyle='--', label='voc' + ref_str)
        plt.plot(mo.time, mo.voc_stat, color='magenta', linestyle='-.', label='voc_stat' + ref_str)
        plt.legend(loc=1)
        plt.subplot(222)
        plt.plot(mo.time, mo.dv_hys, linestyle='-', color='black', label='dv_hys' + ref_str)
        plt.plot(mv.time, mv.dv_hys, linestyle='--', color='orange', label='dv_hys' + test_str)
        plt.legend(loc=1)
        plt.subplot(223)
        plt.plot(mo.time, mo.soc, linestyle='-', color='black', label='soc' + ref_str)
        plt.plot(mv.time, mv.soc, linestyle='--', color='orange', label='soc' + test_str)
        plt.legend(loc=1)
        plt.subplot(224)
        plt.plot(mo.time, mo.ib_sel, linestyle='-', color='red', label='ib_sel' + ref_str)
        plt.plot(so.time, so.ib_in_s, linestyle='--', color='cyan', label='ib_in_s' + ref_str)
        plt.plot(mo.time, mo.ib_charge, linestyle='-.', color='blue', label='ib_charge' + ref_str)
        plt.plot(mv.time, mv.ib_charge, linestyle=':', color='orange', label='ib_charge' + test_str)
        plt.plot(smv.time, smv.ib_charge_s, linestyle='-.', color='blue', label='ib_charge_s' + ref_str)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

    return fig_list, fig_files
