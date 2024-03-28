# PlotGP - general purpose plotting
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
from myFilters import InlineExpLag
# below suppresses runtime error display******************
# import os
# os.environ["KIVY_NO_CONSOLELOG"] = "1"
# from kivy.utils import platform  # failed experiment to run BLE data plotting realtime on android
# if platform != 'linux':
#     from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
plt.rcParams.update({'figure.max_open_warning': 0})


def gp_plot(mo, mv, so, sv, smv, filename, fig_files=None, plot_title=None, fig_list=None,
            ref_str='_ref', test_str='_test'):
    fig_list.append(plt.figure())  # GP 1
    plt.subplot(221)
    plt.title(plot_title + ' GP 1')
    if so is not None:
        plt.plot(so.time, so.vb_s, color='black', linestyle='-', label='vb_s' + ref_str)
    plt.plot(smv.time, smv.vb_s, color='orange', linestyle='--', label='vb_s' + test_str)
    if so is not None:
        plt.plot(so.time, so.voc_s, color='blue', linestyle='-.', label='voc_s' + ref_str)
    plt.plot(smv.time, smv.voc_s, color='red', linestyle=':', label='voc_s' + test_str)
    if so is not None:
        plt.plot(so.time, so.voc_stat_s, color='magenta', linestyle='-.', label='voc_stat_s' + ref_str)
    plt.plot(smv.time, smv.voc_stat_s, color='green', linestyle=':', label='voc_stat_s' + test_str)
    plt.legend(loc=1)
    plt.subplot(222)
    if so is not None:
        plt.plot(so.time, so.dv_hys_s, linestyle='-', color='black', label='dv_hys_s' + ref_str)
    plt.plot(smv.time, smv.dv_hys_s, linestyle='--', color='orange', label='dv_hys_s' + test_str)
    plt.legend(loc=1)
    plt.subplot(223)
    if so is not None:
        plt.plot(so.time, so.soc_s, linestyle='-', color='black', label='soc_s' + ref_str)
    plt.plot(smv.time, smv.soc_s, linestyle='--', color='orange', label='soc_s' + test_str)
    plt.legend(loc=1)
    plt.subplot(224)
    if so is not None:
        plt.plot(so.time, so.ib_in_s, linestyle='-', color='blue', label='ib_in_s' + ref_str)
    if smv is not None:
        plt.plot(smv.time, smv.ib_in_s, linestyle='--', color='red', label='ib_in_s' + test_str)
    if hasattr(smv, 'ib_fut_s'):
        plt.plot(smv.time, smv.ib_fut_s, linestyle='-.', color='orange', label='ib_fut_s' + test_str)
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    fig_list.append(plt.figure())  # GP 2
    plt.subplot(221)
    plt.title(plot_title + ' GP 2')
    plt.plot(mo.time, mo.vb, color='black', linestyle='-', label='vb' + ref_str)
    plt.plot(mv.time, mv.vb, color='orange', linestyle='--', label='vb' + test_str)
    plt.plot(mo.time, mo.voc, color='blue', linestyle='-.', label='voc' + ref_str)
    plt.plot(mv.time, mv.voc, color='red', linestyle=':', label='voc' + test_str)
    plt.plot(mo.time, mo.voc_stat, color='cyan', linestyle='-.', label='voc_stat' + ref_str)
    plt.plot(mv.time, mv.voc_stat, color='black', linestyle=':', label='voc_stat' + test_str)
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
    if mo.ib_sel is not None:
        plt.plot(mo.time, mo.ib_sel, linestyle='-', color='black', label='ib_sel' + ref_str)
    if so is not None:
        plt.plot(so.time, so.ib_in_s, linestyle='--', color='cyan', label='ib_in_s' + ref_str)
    plt.plot(mv.time, mv.ib_charge, linestyle='-.', color='orange', label='ib_charge' + test_str)
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    fig_list.append(plt.figure())  # GP 2 nn
    plt.subplot(321)
    plt.title(plot_title + ' GP 2 nn lag')
    plt.plot(mo.time, mo.sat, color='black', linestyle='-', label='sat' + ref_str)
    plt.plot(mv.time, mv.sat, color='orange', linestyle='--', label='sat' + test_str)
    plt.legend(loc=1)
    plt.subplot(322)
    plt.plot(mo.time, mo.voc, color='black', linestyle='-', label='voc' + ref_str)
    plt.plot(mv.time, mv.voc, color='orange', linestyle='--', label='voc' + test_str)
    plt.plot(mo.time, mo.vsat, color='blue', linestyle='-.', label='vsat' + ref_str)
    plt.plot(mv.time, mv.vsat, color='red', linestyle=':', label='vsat' + test_str)
    plt.plot(mo.time, mo.voc_soc, color='cyan', linestyle='-', label='voc_soc' + ref_str)
    plt.plot(mv.time, mv.voc_soc, color='black', linestyle='--', label='voc_soc' + test_str)
    plt.legend(loc=1)
    plt.subplot(323)
    plt.plot(mo.time, mo.soc, color='black', linestyle='-', label='soc' + ref_str)
    plt.plot(mv.time, mv.soc, color='orange', linestyle='--', label='soc' + test_str)
    plt.legend(loc=1)
    plt.subplot(324)
    plt.plot(mo.time, np.array(mo.ib)+10., color='black', linestyle='-', label='ib+10' + ref_str)
    plt.plot(mv.time, np.array(mv.ib)+10., color='orange', linestyle='--', label='ib+10' + test_str)
    plt.plot(mo.time, mo.ib_lag, color='blue', linestyle='-', label='ib_lag' + ref_str)
    plt.plot(mv.time, mv.ib_lag, color='red', linestyle='--', label='ib_lag' + test_str)
    plt.legend(loc=1)
    plt.subplot(325)
    plt.plot(mo.soc, mo.voc, color='black', linestyle='-', label='voc' + ref_str)
    plt.plot(mo.soc, mo.voc_soc, color='red', linestyle='-', label='voc_soc' + ref_str)
    plt.plot(mv.soc, mv.voc_soc, color='orange', linestyle='--', label='voc_soc' + test_str)
    # plt.plot(mv.soc, mv.voc_soc_new, color='magenta', linestyle='-.', label='voc_soc_new' + test_str)
    plt.plot(mo.soc, np.array(mo.voc_soc) - np.array(mo.voc)+13., color='blue', linestyle='-', label='dv' + ref_str + '+13')
    plt.plot(mv.soc, np.array(mv.voc_soc) - np.array(mv.voc)+13., color='orange', linestyle='--', label='dv' + test_str + '+13')
    plt.legend(loc=1)
    plt.subplot(326)
    plt.plot(mo.time, mo.voc, color='black', linestyle='-', label='voc' + ref_str)
    plt.plot(mo.time, mo.voc_soc, color='red', linestyle='-', label='voc_soc' + ref_str)
    plt.plot(mv.time, mv.voc_soc, color='orange', linestyle='--', label='voc_soc' + test_str)
    # plt.plot(mv.soc, mv.voc_soc_new, color='magenta', linestyle='-.', label='voc_soc_new' + test_str)
    plt.plot(mo.time, np.array(mo.voc_soc) - np.array(mo.voc)+13., color='blue', linestyle='-', label='dv' + ref_str + '+13')
    plt.plot(mv.time, np.array(mv.voc_soc) - np.array(mv.voc)+13., color='orange', linestyle='--', label='dv' + test_str + '+13')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    fig_list.append(plt.figure())  # GP 3 Tune
    plt.subplot(331)
    plt.title(plot_title + ' GP 3 Tune')
    plt.plot(mo.time, mo.dv_dyn, color='blue', linestyle='-', label='dv_dyn' + ref_str)
    plt.plot(mv.time, mv.dv_dyn, color='cyan', linestyle='--', label='dv_dyn' + test_str)
    if so is not None:
        plt.plot(so.time, so.dv_dyn_s, color='black', linestyle='-.', label='dv_dyn_s' + ref_str)
    plt.plot(smv.time, smv.dv_dyn_s, color='magenta', linestyle=':', label='dv_dyn_s' + test_str)
    plt.plot(mo.time, mo.dv_hys, color='pink', linestyle='-', label='dv_hys' + ref_str)
    plt.xlabel('sec')
    plt.legend(loc=3)
    plt.subplot(332)
    plt.plot(mo.time, mo.soc, linestyle='-', color='blue', label='soc' + ref_str)
    plt.plot(mv.time, mv.soc, linestyle='--', color='cyan', label='soc' + test_str)
    if so is not None:
        plt.plot(so.time, so.soc_s, linestyle='-.', color='black', label='soc_s' + ref_str)
    plt.plot(smv.time, smv.soc_s, linestyle=':', color='magenta', label='soc_s' + test_str)
    plt.plot(mo.time, mo.soc_ekf, linestyle='-', color='green', label='soc_ekf' + ref_str)
    plt.plot(mv.time, mv.soc_ekf, linestyle='--', color='red', label='soc_ekf' + test_str)
    plt.xlabel('sec')
    plt.legend(loc=4)
    plt.subplot(333)
    if mo.ib_sel is not None:
        plt.plot(mo.time, mo.ib_sel, linestyle='-', color='blue', label='ib_sel' + ref_str)
    # plt.plot(mv.time, mv.ib_in, linestyle='-', color='cyan', label='ib_in'+test_str)
    if so is not None:
        plt.plot(so.time, so.ib_in_s, linestyle='--', color='black', label='ib_in_s' + ref_str)
    plt.plot(smv.time, smv.ib_in_s, linestyle=':', color='red', label='ib_in_s' + test_str)
    plt.xlabel('sec')
    plt.legend(loc=3)
    plt.subplot(334)
    plt.plot(mo.time, mo.voc, linestyle='-', color='blue', label='voc' + ref_str)
    plt.plot(mv.time, mv.voc, linestyle='--', color='cyan', label='voc' + test_str)
    # if so is not None:
    #   plt.plot(so.time, so.voc_s, linestyle='-', color='blue', label='voc_s'+ref_str)
    # plt.plot(smv.time, smv.voc_s, linestyle='--', color='cyan', label='voc_s'+test_str)
    plt.plot(mo.time, mo.voc_stat, color='orange', linestyle='-', label='voc_stat' + ref_str)
    if so is not None:
        plt.plot(so.time, so.voc_stat_s, linestyle='-.', color='blue', label='voc_stat_s' + ref_str)
    plt.plot(smv.time, smv.voc_stat_s, linestyle=':', color='red', label='voc_stat_s' + test_str)
    if so is not None:
        plt.plot(so.time, so.vb_s, linestyle='-', color='black', label='vb_s' + ref_str)
    plt.plot(sv.time, smv.vb_s, linestyle='--', color='pink', label='vb_s' + test_str)
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(335)
    if mo.e_wrap is not None:
        plt.plot(mo.time, mo.e_wrap, color='black', linestyle='-', label='e_wrap' + ref_str)
    plt.plot(mv.time, mv.e_wrap, color='orange', linestyle='--', label='e_wrap' + test_str)
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(336)
    plt.plot(mo.soc, mo.vb, color='blue', linestyle='-', label='vb' + ref_str)
    plt.plot(smv.soc_s, smv.vb_s, color='cyan', linestyle='--', label='vb_s' + test_str)
    plt.plot(mo.soc, mo.voc_stat, color='orange', linestyle='-.', label='voc_stat' + ref_str)
    plt.plot(smv.soc_s, smv.voc_stat_s, color='red', linestyle=':', label='voc_stat_s' + test_str)
    plt.xlabel('state-of-charge')
    plt.legend(loc=2)
    plt.subplot(337)
    plt.plot(mo.time, mo.vb, color='blue', linestyle='-', label='vb' + ref_str)
    plt.plot(mv.time, mv.vb, color='cyan', linestyle='--', label='vb' + test_str)
    if so is not None:
        plt.plot(so.time, so.vb_s, color='black', linestyle='-.', label='vb_s' + ref_str)
    plt.plot(smv.time, smv.vb_s, color='magenta', linestyle=':', label='vb_s' + test_str)
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(338)
    plt.plot(mo.time, mo.dv_hys, color='blue', linestyle='-', label='dv_hys' + ref_str)
    plt.plot(mv.time, mv.dv_hys, color='cyan', linestyle='--', label='dv_hys' + test_str)
    if so is not None:
        plt.plot(so.time, so.dv_hys_s, color='black', linestyle='-.', label='dv_hys_s' + ref_str)
    plt.plot(smv.time, smv.dv_hys_s, color='magenta', linestyle=':', label='dv_hys_s' + test_str)
    plt.plot(mo.time, mo.sat - 0.5, color='black', linestyle='-', label='sat' + ref_str + '-0.5')
    plt.plot(mv.time, np.array(mv.sat) - 0.5, color='green', linestyle='--', label='sat' + test_str + '-0.5')
    if so is not None:
        plt.plot(so.time, so.sat_s - 0.5, color='red', linestyle='-.', label='sat_s' + ref_str + '-0.5')
    plt.plot(sv.time, np.array(sv.sat) - 0.5, color='cyan', linestyle=':', label='sat_s' + test_str + '-0.5')
    plt.xlabel('sec')
    plt.legend(loc=3)
    plt.subplot(339)
    plt.plot(mo.time, mo.Tb, color='blue', linestyle='-', label='Tb' + ref_str)
    # plt.plot(mv.time, mv.tau_hys, color='cyan', linestyle='--', label='tau_hys' + test_str)
    plt.legend(loc=3)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return fig_list, fig_files


def tune_r(mo, mv, smv, filename, fig_files=None, plot_title=None, fig_list=None, ref_str='_ref', test_str='_test'):
    # delineate charging and discharging
    voc_stat_chg = np.copy(mv.voc_stat)
    voc_stat_dis = np.copy(mv.voc_stat)
    if hasattr(smv, 'ib_in_s'):
        for i in range(len(voc_stat_chg)):
            if smv.ib_in_s[i] > -0.1:
                voc_stat_dis[i] = None
            elif smv.ib_in_s[i] < 0.1:
                voc_stat_chg[i] = None

    vb = np.copy(mv.vb)
    voc = np.copy(mv.voc)
    voc_soc = np.copy(mv.voc_soc)
    voc_stat = np.copy(mv.voc_stat)
    ib_f = np.copy(mv.ib)
    ioc_f = np.copy(mv.ioc)
    t = np.copy(mv.time)
    dv_hys_calc = voc - voc_stat  # assumes Charge Transfer tuned
    dv_hys_req = voc - voc_soc
    dv_hys_calc_f = np.copy(dv_hys_calc)
    dv_hys_req_f = np.copy(dv_hys_req)
    dv_dot_calc = np.copy(dv_hys_calc)
    dv_dot_req = np.copy(dv_hys_req)
    dv_dot_cap = np.copy(dv_hys_calc)
    dv_bleed = np.copy(dv_hys_calc)
    ioc_req = np.copy(dv_hys_req)
    r_calc = np.copy(dv_hys_req)
    r_calc_from_dot = np.copy(dv_hys_req)
    r_req = np.copy(dv_hys_req)
    ioc_calc_from_dot = np.copy(dv_hys_req)

    tau = 20
    cap = 1000
    n = len(dv_hys_req)
    dv_hys_calc_filter = InlineExpLag(tau)
    dv_hys_req_filter = InlineExpLag(tau)
    ib_filter = InlineExpLag(tau)
    ios_filter = InlineExpLag(tau)
    dv_hys_dot_filter = InlineExpLag(tau)
    for i in range(n-1):
        reset = i == 0
        T = t[i+1]-t[i]

        dv_hys_calc_f[i] = dv_hys_calc_filter.update(dv_hys_calc[i], T, reset=reset)
        dv_hys_req_f[i] = dv_hys_req_filter.update(dv_hys_req[i], T, reset=reset)
        ib_f[i] = ib_filter.update(mv.ib[i], T, reset=reset)
        ioc_f[i] = ios_filter.update(mv.ioc[i], T, reset=reset)

        dv_dot_calc[i] = dv_hys_calc_filter.rate
        # dv_dot_req[i] = dv_hys_req_filter.rate
        dv_dot_req[i] = dv_hys_dot_filter.update(dv_hys_req_filter.rate, T, reset=reset)
        dv_dot_cap[i] = ib_f[i] / cap
        dv_bleed[i] = dv_dot_cap[i] - dv_dot_req[i]

        ioc_req[i] = dv_bleed[i] * cap
        if abs(ib_f[i]) < 0.5:
            r_req[i] = 0
        else:
            # noinspection PyTypeChecker
            r_req[i] = max(min(dv_hys_req_f[i] / ioc_req[i], 0.1), -0.1)

        ioc_calc_from_dot[i] = ib_f[i] - cap*dv_dot_calc[i]
        if abs(ioc_calc_from_dot[i]) > 1e-9:
            # noinspection PyTypeChecker
            r_calc_from_dot[i] = max(min(dv_hys_calc_f[i] / ioc_calc_from_dot[i], 0.1), -0.1)
        else:
            r_calc_from_dot[i] = 0.
        # ioc_calc_from_dot[i] = mv.ib[i] - cap*dv_dot_calc[i]
        # r_calc_from_dot[i] = max(min(dv_hys_calc[i] / ioc_calc_from_dot[i], 0.1), -0.1)

        if abs(ioc_f[i]) < .5:
            r_calc[i] = 0
        else:
            # noinspection PyTypeChecker
            r_calc[i] = max(min(dv_hys_calc_f[i] / ioc_f[i], 0.1), -0.1)

    dv_dot_calc[n-1] = dv_dot_calc[n-2]
    dv_dot_req[n-1] = dv_dot_req[n-2]
    r_calc[n-1] = r_calc[n-2]
    r_calc_from_dot[n-1] = r_calc_from_dot[n-2]
    ioc_req[n-1] = ioc_req[n-2]
    dv_hys_req_f[n-1] = dv_hys_req_f[n-2]
    dv_hys_calc_f[n-1] = dv_hys_calc_f[n-2]
    ioc_calc_from_dot[n-1] = ioc_calc_from_dot[n-2]
    ib_f[n-1] = ib_f[n-2]
    ioc_f[n-1] = ioc_f[n-2]
    dv_dot_cap[n-1] = dv_dot_cap[n-2]
    dv_bleed[-1] = dv_bleed[-2]

    fig_list.append(plt.figure())  # GP 3 Tune R
    plt.subplot(321)
    plt.title(plot_title + ' GP 3 Tune R')
    plt.plot(t, vb, color='blue', linestyle='-', label='vb_x')
    if hasattr(smv, 'vb_s'):
        plt.plot(t, smv.vb_s, color='cyan', linestyle='--', label='vb_s_ver')
    plt.plot(t, voc, color='magenta', linestyle='-.', label='voc_x')
    plt.plot(t, voc_soc, color='black', linestyle=':', label='voc_soc_x')
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(322)
    plt.plot(t, mo.dv_dyn, color='red', linestyle='-', label='dv_dyn_x')
    plt.plot(t, mv.dv_dyn, color='black', linestyle='-', label='dv_dyn_ver')
    plt.plot(t, dv_hys_calc, color='blue', linestyle='-', label='dv_hys_calc_x')
    plt.plot(t, dv_hys_req, color='magenta', linestyle='--', label='dv_hys_req_x')
    plt.plot(t, dv_hys_calc_f, color='red', linestyle='-', label='dv_hys_calc_f_x')
    plt.plot(t, dv_hys_req_f, color='cyan', linestyle='--', label='dv_hys_req_f_x')
    plt.plot(t, mv.dv_hys, color='orange', linestyle=':', label='dv_hys_x')
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(323)
    plt.plot(t, r_calc_from_dot, color='cyan', linestyle='--', label='r_calc_from_dot_x')
    plt.plot(t, r_calc, color='blue', linestyle='-', label='r_calc_x')
    plt.plot(t, r_req, color='black', linestyle='-', label='r_req_x')
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(324)
    plt.plot(t, ioc_calc_from_dot, color='orange', linestyle='--', label='ioc_calc_from_dot_x')
    plt.plot(t, mv.ib, color='blue', linestyle='-', label='ib_x')
    plt.plot(t, ib_f, color='cyan', linestyle='--', label='ib_f_x')
    plt.plot(t, mv.ioc, color='red', linestyle='-', label='ioc_x')
    plt.plot(t, ioc_f, color='pink', linestyle='--', label='ioc_f_x')
    plt.plot(t, ioc_req, color='magenta', linestyle=':', label='ioc_req_x')
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(325)
    plt.plot(t, ioc_req, color='magenta', linestyle='--', label='ioc_req_x')
    plt.plot(t, mv.ib, color='blue', linestyle='-', label='ib_x')
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(326)
    plt.plot(t, dv_dot_req, color='magenta', linestyle='-', label='dv_dot_req_x')
    plt.plot(t, dv_dot_cap, color='black', linestyle='--', label='dv_dot_cap_x')
    plt.plot(t, dv_dot_calc, color='blue', linestyle='-.', label='dv_dot_calc_x')
    plt.xlabel('sec')
    plt.legend(loc=2)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    fig_list.append(plt.figure())  # GP 3 Tune Summ
    plt.subplot(221)
    plt.title(plot_title + ' GP 3 Tune Summ')
    plt.plot(mo.time, mo.vb, color='blue', linestyle='-', label='vb' + ref_str)
    plt.plot(smv.time, smv.vb_s, color='magenta', linestyle=':', label='vb_s' + test_str)
    plt.plot(smv.time, smv.voc_stat_s, linestyle='-.', color='black', label='voc_stat_s' + test_str)
    plt.plot(mv.time, voc_stat_chg, linestyle=':', color='green', label='voc_stat_chg' + test_str)
    plt.plot(mv.time, voc_stat_dis, linestyle=':', color='red', label='voc_stat_dis' + test_str)
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(222)
    plt.plot(mo.soc, mo.vb, color='blue', linestyle='-', label='vb' + ref_str)
    plt.plot(smv.soc_s, smv.vb_s, color='magenta', linestyle=':', label='vb_s' + test_str)
    plt.plot(smv.soc_s, smv.voc_stat_s, linestyle='-.', color='black', label='voc_stat_s' + test_str)
    plt.plot(mv.soc, voc_stat_chg, linestyle=':', color='green', label='voc_stat_chg' + test_str)
    plt.plot(mv.soc, voc_stat_dis, linestyle=':', color='red', label='voc_stat_dis' + test_str)
    plt.plot(mo.soc, mo.voc_soc, color='cyan', linestyle='--', label='voc_soc' + ref_str)
    plt.xlabel('state-of-charge')
    plt.legend(loc=2)
    plt.subplot(223)
    plt.plot(mo.time, mo.Tb, color='red', linestyle='-', label='Tb' + ref_str)
    plt.plot(mo.time, mo.ib_sel, linestyle='--', color='blue', label='ib_sel' + ref_str)
    plt.plot(smv.time, smv.ib_in_s, linestyle='-.', color='magenta', label='ib_in_s' + test_str)
    plt.xlabel('sec')
    plt.legend(loc=3)
    plt.subplot(224)
    plt.plot(smv.time, smv.dv_dyn_s, color='black', linestyle=':', label='dv_dyn_s' + test_str)
    plt.plot(smv.time, smv.dv_hys_s, color='magenta', linestyle=':', label='dv_hys_s' + test_str)
    plt.xlabel('sec')
    plt.legend(loc=3)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return fig_list, fig_files
