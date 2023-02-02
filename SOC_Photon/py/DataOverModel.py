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
from Battery import overall_batt, IB_MIN_UP
# below suppresses runtime error display******************
# import os
# os.environ["KIVY_NO_CONSOLELOG"] = "1"
# from kivy.utils import platform  # failed experiment to run BLE data plotting realtime on android
# if platform != 'linux':
#     from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
plt.rcParams.update({'figure.max_open_warning': 0})


def overall(mo, mv, so, sv, smv, filename, fig_files=None, plot_title=None, n_fig=None, plot_init_in=False,
            old_str='_old', new_str='_new'):
    if fig_files is None:
        fig_files = []

    if plot_init_in:
        plt.figure()  # init 1
        n_fig += 1
        plt.subplot(221)
        plt.title(plot_title + ' init 1')
        plt.plot(so.time, so.reset_s, color='black', linestyle='-', label='reset_s'+old_str)
        plt.plot(smv.time, smv.reset_s, color='red', linestyle='--', label='reset_s'+new_str)
        plt.plot(mo.time, mo.reset, color='magenta', linestyle='-', label='reset'+old_str)
        plt.plot(mv.time, mv.reset, color='cyan', linestyle='--', label='reset'+new_str)
        plt.legend(loc=1)
        plt.subplot(222)
        plt.plot(so.time, so.Tb_s, color='black', linestyle='-', label='Tb_s'+old_str)

        plt.plot(sv.time, sv.Tb, color='red', linestyle='--', label='Tb_s'+new_str)
        plt.plot(mo.time, mo.Tb, color='blue', linestyle='-.', label='Tb'+old_str)
        plt.plot(mv.time, mv.Tb, color='green', linestyle=':', label='Tb'+new_str)
        plt.legend(loc=1)
        plt.subplot(223)
        plt.plot(so.time, so.soc_s, color='black', linestyle='-', label='soc_s'+old_str)
        plt.plot(sv.time, sv.soc, color='red', linestyle='--', label='soc_s'+new_str)
        plt.plot(mo.time, mo.soc, color='blue', linestyle='-.', label='soc'+old_str)
        plt.plot(mv.time, mv.soc, color='green', linestyle=':', label='soc'+new_str)
        plt.plot(mo.time, mo.soc_ekf, marker='^', markersize='5', markevery=32, linestyle='None', color='orange', label='soc_ekf'+old_str)
        plt.plot(mv.time, mv.soc_ekf, marker='+', markersize='5', markevery=32, linestyle='None', color='cyan',  label='soc_ekf'+new_str)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        return n_fig, fig_files

    if mo.ibmh is not None:
        plt.figure()  # 1a
        n_fig += 1
        plt.subplot(321)
        plt.title(plot_title + ' 1a')
        plt.plot(mo.time, mo.ibmh, color='black', linestyle='-', label='ib_amp_hdwe'+old_str)
        plt.plot(mo.time, mo.ibnh, color='green', linestyle='--', label='ib_noa_hdwe'+old_str)
        plt.plot(mo.time, mo.ib_sel, color='red', linestyle='-.', label='ib_sel'+old_str)
        plt.plot(mo.time, mo.ib_charge, linestyle=':', color='blue', label='ib_charge'+old_str)
        plt.legend(loc=1)
        plt.subplot(322)
        plt.plot(mo.time, mo.ib_diff, color='black', linestyle='-', label='ib_diff'+old_str)
        plt.plot(mo.time, mo.ib_diff_f, color='magenta', linestyle='--', label='ib_diff_f'+old_str)
        plt.plot(mo.time, mo.ibd_thr, color='red', linestyle='-.', label='ib_diff_thr'+old_str)
        plt.plot(mo.time, -mo.ibd_thr, color='red', linestyle='-.')
        plt.legend(loc=1)
        plt.subplot(323)
        plt.plot(mo.time, mo.ib_quiet, color='green', linestyle='-', label='ib_quiet'+old_str)
        plt.plot(mo.time, mo.ibq_thr, color='red', linestyle='--', label='ib_quiet_thr'+old_str)
        plt.plot(mo.time, -mo.ibq_thr, color='red', linestyle='--')
        plt.plot(mo.time, mo.dscn_fa-.02, color='magenta', linestyle='-.', label='dscn_fa'+old_str+'-0.02')
        plt.ylim(-.1, .1)
        plt.legend(loc=1)
        plt.subplot(324)
        plt.plot(mo.time, mo.ib_rate, color='orange', linestyle='-', label='ib_rate'+old_str)
        plt.plot(mo.time, mo.ib_quiet, color='green', linestyle='--', label='ib_quiet'+old_str)
        plt.plot(mo.time, mo.ibq_thr, color='red', linestyle='-.', label='ib_quiet_thr'+old_str)
        plt.plot(mo.time, -mo.ibq_thr, color='red', linestyle='-.')
        plt.ylim(-6, 6)
        plt.legend(loc=1)
        plt.subplot(325)
        plt.plot(mo.time, mo.e_wrap, color='magenta', linestyle='-', label='e_wrap'+old_str)
        plt.plot(mo.time, mo.e_wrap_filt, color='black', linestyle='--', label='e_wrap_filt'+old_str)
        plt.legend(loc=1)
        plt.subplot(326)
        plt.plot(mo.time, mo.cc_dif, color='black', linestyle='-', label='cc_diff'+old_str)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # DOM 1
        n_fig += 1
        plt.subplot(331)
        plt.title(plot_title + ' DOM 1')
        plt.plot(mo.time, mo.ib_charge, color='green', linestyle='-', label='ib_charge'+old_str)
        plt.plot(mv.time, mv.ib_charge, linestyle='--', color='red', label='ib_charge'+new_str)
        plt.plot(mo.time, mo.ib_diff, color='black', linestyle='-.', label='ib_diff'+old_str)
        plt.plot(mo.time, mo.ib_diff_f, color='magenta', linestyle=':', label='ib_diff_f'+old_str)
        plt.plot(mo.time, mo.ibd_thr, color='black', linestyle='--', label='ib_diff_thr'+old_str)
        plt.plot(mo.time, -mo.ibd_thr, color='black', linestyle='--')
        plt.legend(loc=1)
        plt.subplot(338)
        mod_min = min(min(mo.mod_data), min(mv.mod_data))
        plt.plot(mo.time, mo.mod_data-mod_min, color='blue', linestyle='-', label='mod'+old_str+'-'+str(mod_min))
        plt.plot(mv.time, mv.mod_data-mod_min, color='red', linestyle='--', label='mod'+new_str+'-'+str(mod_min))
        plt.plot(mo.time, mo.sat+2, color='black', linestyle='-',  label='sat'+old_str+'+2')
        plt.plot(mv.time, np.array(mv.sat)+2, color='green', linestyle='--', label='sat'+new_str+'+2')
        plt.plot(mo.time, np.array(mo.bms_off)+4, color='red', linestyle='-', label='bms_off'+old_str+'+4')
        plt.plot(mv.time, np.array(mv.bms_off)+4, color='green', linestyle='--', label='bms_off'+new_str+'+4')
        if so is not None:
            plt.plot(so.time, np.array(so.bms_off_s)+4, color='blue', linestyle='-.', label='bms_off_s'+old_str+'+4')
        if smv is not None:
            if hasattr(smv, 'bmso_s'):
                plt.plot(smv.time, np.array(smv.bmso_s)+4, color='orange', linestyle=':', label='bms_off_s'+new_str+'+4')
            elif hasattr(smv, 'bms_off_s'):
                plt.plot(smv.time, np.array(smv.bms_off_s) + 4, color='orange', linestyle=':', label='bms_off_s' + new_str + '+4')
        plt.plot(mo.time, mo.sel, color='red', linestyle='-.', label='sel'+old_str)
        plt.plot(mv.time, mv.sel, color='blue', linestyle=':', label='sel'+new_str)
        plt.plot(mo.time, mo.ib_sel_stat-2, color='black', linestyle='-', label='ib_sel_stat'+old_str+'-2')
        plt.plot(mo.time, mo.vb_sel-2, color='green', linestyle='--', label='vb_sel_stat'+old_str+'-2')
        plt.plot(mo.time, mo.preserving-2, color='cyan', linestyle='-.', label='preserving'+old_str+'-2')
        plt.legend(loc=1)
        plt.subplot(333)
        plt.plot(mo.time, mo.vb, color='green', linestyle='-', label='vb'+old_str)
        plt.plot(mv.time, mv.vb, color='orange', linestyle='--', label='vb'+new_str)
        plt.legend(loc=1)
        plt.subplot(334)
        plt.plot(mo.time, mo.voc_stat, color='green', linestyle='-', label='voc_stat'+old_str)
        plt.plot(mv.time, mv.voc_stat, color='orange', linestyle='--', label='voc_stat'+new_str)
        plt.plot(mo.time, mo.vsat, color='blue', linestyle='-', label='vsat'+old_str)
        plt.plot(mv.time, mv.vsat, color='red', linestyle='--', label='vsat'+new_str)
        plt.plot(mo.time, mo.voc_soc+0.1*1, color='black', linestyle='-', label='voc_soc'+old_str+'+0.1')
        plt.plot(mo.time, mo.voc+0.1*1, color='cyan', linestyle='--', label='voc'+old_str+'+0.1')
        plt.plot(mv.time, np.array(mv.voc)+0.1*1, color='red', linestyle='-.', label='voc'+new_str+'+0.1')
        plt.legend(loc=1)
        plt.subplot(335)
        plt.plot(mo.time, mo.e_wrap, color='magenta', linestyle='-', label='e_wrap'+old_str)
        plt.plot(mv.time, mv.e_wrap, color='cyan', linestyle='--', label='e_wrap'+new_str)
        plt.plot(mo.time, mo.e_wrap_filt, color='black', linestyle='-.', label='e_wrap_filt'+old_str)
        plt.plot(mv.time, mv.e_wrap_filt, color='red', linestyle=':', label='e_wrap_filt'+new_str)
        plt.plot(mo.time, mo.ewh_thr, color='red', linestyle='-.', label='ewh_thr'+old_str)
        plt.plot(mo.time, mo.ewl_thr, color='red', linestyle='-.', label='ewl_thr'+old_str)
        plt.ylim(-1, 1)
        plt.legend(loc=1)
        plt.subplot(336)
        plt.plot(mo.time, mo.wh_flt+4, color='black', linestyle='-', label='wrap_hi_flt'+old_str+'+4')
        plt.plot(mo.time, mo.wl_flt+4, color='magenta', linestyle='--', label='wrap_lo_flt'+old_str+'+4')
        plt.plot(mo.time, mo.wh_fa+2, color='cyan', linestyle='-', label='wrap_hi_fa'+old_str+'+2')
        plt.plot(mo.time, mo.wl_fa+2, color='red', linestyle='--', label='wrap_lo_fa'+old_str+'+2')
        plt.plot(mo.time, mo.wv_fa+2, color='orange', linestyle='-.', label='wrap_vb_fa'+old_str+'+2')
        plt.plot(mo.time, mo.ccd_fa, color='green', linestyle='-', label='cc_diff_fa'+old_str)
        plt.plot(mo.time, mo.red_loss, color='blue', linestyle='--', label='red_loss'+old_str)
        plt.legend(loc=1)
        plt.subplot(337)
        plt.plot(mo.time, mo.cc_dif, color='black', linestyle='-', label='cc_diff'+old_str)
        plt.plot(mo.time, mo.ccd_thr, color='red', linestyle='--', label='cc_diff_thr'+old_str)
        plt.plot(mo.time, -mo.ccd_thr, color='red', linestyle='--')
        plt.ylim(-.01, .01)
        plt.legend(loc=1)
        plt.subplot(332)
        # plt.plot(mo.time, mo.ib_rate, color='orange', linestyle='--', label='ib_rate'+old_str)
        plt.plot(mo.time, mo.ib_quiet, color='black', linestyle='-', label='ib_quiet'+old_str)
        plt.plot(mo.time, mo.ibq_thr, color='red', linestyle='--', label='ib_quiet_thr'+old_str)
        plt.plot(mo.time, -mo.ibq_thr, color='red', linestyle='--')
        plt.plot(mo.time, mo.dscn_flt-4, color='blue', linestyle='-.', label='dscn_flt'+old_str+'-4')
        plt.plot(mo.time, mo.dscn_fa+4, color='red', linestyle='-', label='dscn_fa'+old_str+'+4')
        plt.legend(loc=1)
        plt.subplot(339)
        plt.plot(mo.time, mo.ib_diff_flt+2, color='cyan', linestyle='-', label='ib_diff_flt'+old_str+'+2')
        plt.plot(mo.time, mo.ib_diff_fa+2, color='magenta', linestyle='--', label='ib_diff_fa'+old_str+'+2')
        plt.plot(mo.time, mo.vb_flt, color='blue', linestyle='-.', label='vb_flt'+old_str)
        plt.plot(mo.time, mo.vb_fa, color='black', linestyle=':', label='vb_fa'+old_str)
        plt.plot(mo.time, mo.tb_flt, color='red', linestyle='-', label='tb_flt'+old_str)
        plt.plot(mo.time, mo.tb_fa, color='cyan', linestyle='--', label='tb_fa'+old_str)
        plt.plot(mo.time, mo.ib_sel_stat-2, color='black', linestyle='-', label='ib_sel_stat'+old_str+'-2')
        plt.plot(mo.time, mo.vb_sel-2, color='green', linestyle='--', label='vb_sel_stat'+old_str+'-2')
        plt.plot(mo.time, mo.tb_sel-2, color='red', linestyle='-.', label='tb_sel_stat'+old_str+'-2')
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

    plt.figure()  # DOM 2
    n_fig += 1
    plt.subplot(321)
    plt.title(plot_title + ' DOM 2')
    plt.plot(mo.time, mo.dv_dyn, color='green', linestyle='-', label='dv_dyn'+old_str)
    plt.plot(mv.time, mv.dv_dyn, color='orange', linestyle='--', label='dv_dyn'+new_str)
    plt.legend(loc=1)
    plt.subplot(322)
    plt.plot(mo.time, mo.voc_stat, color='green', linestyle='-', label='voc_stat'+old_str)
    plt.plot(mv.time, mv.voc_stat, color='orange', linestyle='--', label='voc_stat'+new_str)
    plt.legend(loc=1)
    plt.subplot(323)
    plt.plot(mo.time, mo.voc, color='green', linestyle='-', label='voc'+old_str)
    plt.plot(mv.time, mv.voc, color='orange', linestyle='--', label='voc'+new_str)
    plt.plot(mo.time, mo.voc_ekf, color='blue', linestyle='-.', label='voc_ekf'+old_str)
    plt.plot(mv.time, mv.voc_ekf, color='red', linestyle=':', label='voc_ekf'+new_str)
    plt.legend(loc=1)
    plt.subplot(324)
    plt.plot(mo.time, mo.y_ekf, color='green', linestyle='-', label='y_ekf'+old_str)
    plt.plot(mv.time, mv.y_ekf, color='orange', linestyle='--', label='y_ekf'+new_str)
    if hasattr(mv, 'y_filt'):
        plt.plot(mv.time, mv.y_filt, color='black', linestyle='-.', label='y_filt'+new_str)
    if hasattr(mv, 'y_filt2'):
        plt.plot(mv.time, mv.y_filt2, color='cyan', linestyle=':', label='y_filt2'+new_str)
    plt.legend(loc=1)
    plt.subplot(325)
    plt.plot(mo.time, mo.dv_hys, color='green', linestyle='-', label='dv_hys'+old_str)
    plt.plot(mv.time, mv.dv_hys, color='cyan', linestyle='--', label='dv_hys'+new_str)
    if sv:
        from pyDAGx import myTables
        lut_vb = myTables.TableInterp1D(np.array(mo.time), np.array(mo.vb))
        n = len(sv.time)
        voc_req = np.zeros((n, 1))
        dv_hys_req = np.zeros((n, 1))
        voc_stat_req = np.zeros((n, 1))
        for i in range(n):
            voc_req[i] = lut_vb.interp(sv.time[i]) - smv.dv_dyn_s[i]
            voc_stat_req[i] = smv.voc_stat_s[i]
            dv_hys_req[i] = voc_req[i] - smv.voc_stat_s[i]
        plt.plot(smv.time, np.array(smv.dv_hys_s)+0.1, color='red', linestyle='-', label='dv_hys_s'+new_str+'+0.1')
        plt.plot(sv.time, np.array(dv_hys_req)+0.1, color='black', linestyle='--', label='dv_hys_req_s'+new_str+'+0.1')

    if so is not None:
        from pyDAGx import myTables
        lut_vb = myTables.TableInterp1D(np.array(mo.time), np.array(mo.vb))
        n = len(so.time)
        voc_req = np.zeros((n, 1))
        dv_hys_req = np.zeros((n, 1))
        voc_stat_req = np.zeros((n, 1))
        for i in range(n):
            voc_req[i] = lut_vb.interp(so.time[i]) - so.dv_dyn_s[i]
            voc_stat_req[i] = so.voc_stat_s[i]
            dv_hys_req[i] = voc_req[i] - so.voc_stat_s[i]
        plt.plot(so.time, np.array(so.dv_hys_s)-0.1, color='magenta',  linestyle='-', label='dv_hys_s-0.1'+old_str)
        plt.plot(so.time, np.array(dv_hys_req)-0.1, color='orange', linestyle='--', label='dv_hys_req_s-0.1'+old_str)
    plt.legend(loc=1)
    plt.subplot(326)
    plt.plot(mo.time, mo.Tb, color='green', linestyle='-', label='temp_c'+old_str)
    plt.plot(mv.time, mv.Tb, color='orange', linestyle='--', label='temp_c'+new_str)
    plt.plot(mo.time, mo.chm, color='black', linestyle='-', label='mon_mod'+old_str)
    if so is not None:
        plt.plot(so.time, so.chm_s, color='cyan', linestyle='--', label='sim_mod'+old_str)
    plt.ylim(0., 50.)
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # DOM 3
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title + ' DOM 3')
    plt.plot(mo.time, mo.soc, color='blue', linestyle='-', label='soc'+old_str)
    plt.plot(mv.time, mv.soc, color='red', linestyle='--', label='soc'+new_str)
    plt.legend(loc=1)
    plt.subplot(222)
    plt.plot(mo.time, mo.soc_ekf, color='green', linestyle='-', label='soc_ekf'+old_str)
    plt.plot(mv.time, mv.soc_ekf, color='orange', linestyle='--', label='soc_ekf'+new_str)
    plt.legend(loc=1)
    plt.subplot(223)
    plt.plot(mo.time, mo.soc_s, color='blue', linestyle='-.', label='soc_s'+old_str)
    plt.plot(mv.time, mv.soc_s, color='red', linestyle=':', label='soc_s'+new_str)
    plt.legend(loc=1)
    plt.subplot(224)
    plt.plot(mo.time, mo.soc, color='blue', linestyle='-', label='soc'+old_str)
    plt.plot(mv.time, mv.soc, color='red', linestyle='--', label='soc'+new_str)
    plt.plot(mo.time, mo.soc_s, color='green', linestyle='-.', label='soc_s'+old_str)
    plt.plot(mv.time, mv.soc_s, color='orange', linestyle=':', label='soc_s'+new_str)
    plt.plot(mo.time, mo.soc_ekf, color='cyan', linestyle='-', label='soc_ekf'+old_str)
    plt.plot(mv.time, mv.soc_ekf, color='black', linestyle='--', label='soc_ekf'+new_str)
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # DOM 4
    n_fig += 1
    plt.subplot(131)
    plt.title(plot_title + ' DOM 4')
    plt.plot(mo.time, mo.soc, color='orange', linestyle='-', label='soc'+old_str)
    plt.plot(mv.time, mv.soc, color='green', linestyle='--', label='soc'+new_str)
    plt.plot(smv.time, smv.soc_s, color='black', linestyle='-.', label='soc_s'+new_str)
    plt.plot(mo.time, mo.soc_ekf, color='red', linestyle=':', label='soc'+old_str)
    plt.plot(mv.time, mv.soc_ekf, color='cyan', linestyle=':', label='soc_ekf'+new_str)
    plt.legend(loc=1)
    plt.subplot(132)
    plt.plot(mo.time, mo.vb, color='orange', linestyle='-', label='vb'+old_str)
    if mo.vb_h is not None:
        plt.plot(mo.time, mo.vb_h, color='cyan', linestyle='--', label='vb_hdwe'+old_str)
    plt.plot(mv.time, mv.vb, color='green', linestyle='-.', label='vb'+new_str)
    plt.plot(smv.time, smv.vb_s, color='black', linestyle='-.', label='vb_s'+new_str)
    plt.legend(loc=1)
    plt.subplot(133)
    plt.plot(mo.soc, mo.vb, color='orange', linestyle='-', label='vb'+old_str)
    if mo.vb_h is not None:
        plt.plot(mo.soc, mo.vb_h, color='cyan', linestyle='--', label='vb_hdwe'+old_str)
    plt.plot(mv.soc, mv.vb, color='green', linestyle='-.', label='vb'+new_str)
    plt.plot(smv.soc_s, smv.vb_s, color='black', linestyle='-.', label='vb_s'+new_str)
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    if so and smv:
        plt.figure()  # sim_s  1
        n_fig += 1
        plt.subplot(331)
        plt.title(plot_title + ' sim_s 1')
        plt.plot(mo.time, mo.ib_sel, color='blue',  linestyle='-', label='ib_sel=ib_in'+old_str)
        if hasattr(mv, 'ib_in'):
            plt.plot(mv.time, mv.ib_in, color='cyan', linestyle='--', label='ib_in'+new_str)
        plt.plot(so.time, so.ib_in_s, color='green', linestyle='-.', label='ib_in_s'+old_str)
        plt.plot(smv.time, smv.ib_in_s, color='orange', linestyle=':', label='ib_in_s'+new_str)
        plt.plot(mo.time, np.array(mo.ib_charge)-1, color='black',  linestyle='-', label='ib_charge'+old_str+'-1')
        plt.plot(mv.time, np.array(mv.ib_charge)-1, color='orange', linestyle='--', label='ib_charge'+new_str+'-1')
        plt.plot(so.time, np.array(so.ib_charge_s)-1, color='blue', linestyle='-.', label='ib_charge_s'+old_str+'-1')
        plt.plot(smv.time, np.array(smv.ib_charge_s)-1, color='red', linestyle=':', label='ib_charge_s'+new_str+'-1')
        plt.legend(loc=1)
        plt.subplot(332)
        plt.plot(so.time, so.soc_s, color='magenta', linestyle='-', label='soc_s'+old_str)
        plt.plot(smv.time, smv.soc_s, color='green', linestyle='--', label='soc_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(333)
        plt.plot(so.time, so.voc_stat_s, color='magenta', linestyle='-', label='voc_stat_s'+old_str)
        plt.plot(smv.time, smv.voc_stat_s, color='green', linestyle='--', label='voc_stat_s'+new_str)
        plt.plot(so.time, so.vsat_s, color='blue',  linestyle='-.', label='vsat_s'+old_str)
        plt.plot(smv.time, smv.vsat_s, color='cyan', linestyle=':', label='vsat_s'+new_str)
        plt.plot(so.time, so.vb_s, color='orange', linestyle='-.', label='vb_s'+old_str)
        plt.plot(smv.time, smv.vb_s, color='black', linestyle=':', label='vb_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(334)
        plt.plot(so.time, so.Tb_s, color='magenta', linestyle='-', label='Tb_s'+old_str)
        plt.plot(smv.time, smv.Tb_s, color='green', linestyle='--', label='Tb_s'+new_str)
        plt.plot(so.time, so.Tbl_s, color='blue', linestyle='-.', label='Tbl_s'+old_str)
        plt.plot(smv.time, smv.Tbl_s, color='cyan', linestyle=':', label='Tbl_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(335)
        plt.plot(so.time, so.dv_dyn_s, color='magenta', linestyle='-', label='dv_dyn_s'+old_str)
        plt.plot(smv.time, smv.dv_dyn_s, color='green', linestyle='--', label='dv_dyn_s'+new_str)
        plt.plot(mo.time, mo.dv_dyn, color='blue', linestyle='-.', label='dv_dyn'+old_str)
        plt.plot(mv.time, mv.dv_dyn, color='cyan', linestyle=':', label='dv_dyn'+new_str)
        plt.legend(loc=1)
        plt.subplot(336)
        plt.plot(mo.time, mo.ib_sel, color='blue', linestyle='-', label='ib_sel'+old_str)
        plt.plot(so.time, so.ib_s, color='red', linestyle='--', label='ib_s'+old_str)
        plt.plot(mo.time, mo.ioc, color='cyan', linestyle='-', label='ioc'+old_str)
        plt.plot(mv.time, mv.ioc, color='magenta', linestyle='--', label='ioc'+new_str)
        plt.plot(so.time, so.ioc_s, color='green', linestyle='-.', label='ioc_s'+old_str)
        if hasattr(sv, 'ioc'):
            plt.plot(sv.time, sv.ioc, color='black', linestyle=':', label='ioc_s'+new_str)
        elif hasattr(sv, 'ioc_s'):
            plt.plot(sv.time, sv.ioc_s, color='black', linestyle=':', label='ioc_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(337)
        plt.plot(so.time, so.dq_s, color='magenta', linestyle='-', label='dq_s'+old_str)
        plt.plot(smv.time, smv.dq_s, color='green', linestyle='--', label='dq_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(338)
        plt.plot(so.time, so.reset_s, color='magenta', linestyle='-', label='reset_s'+old_str)
        plt.plot(smv.time, smv.reset_s, color='green', linestyle='--', label='reset_s'+new_str)
        plt.plot(so.time, so.sat_s, color='blue', linestyle='-.', label='sat_s'+old_str)
        plt.plot(smv.time, smv.sat_s, color='red', linestyle=':', label='sat_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(339)
        plt.plot(mo.time, mo.chm, color='blue', linestyle='-', label='chem'+old_str)
        plt.plot(mv.time, mv.chm, color='red', linestyle='--', label='chm'+new_str)
        plt.plot(so.time, so.chm_s, color='green', linestyle='-.', label='chem_s'+old_str)
        plt.plot(smv.time, smv.chm_s, color='orange', linestyle=':', label='smv.chm_s'+new_str)
        if hasattr(sv, 'chm'):
            plt.plot(sv.time, np.array(sv.chm)+4, color='red', linestyle='-', label='sv.chm'+new_str+'+4')
        elif hasattr(sv, 'chm_s'):
            plt.plot(sv.time, np.array(sv.chm_s)+4, color='red', linestyle='-', label='sv.chm'+new_str+'+4')
        plt.plot(smv.time, np.array(smv.chm_s)+4, color='black', linestyle='--', label='smv.chm_s'+new_str+'+4')
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # sim_s  2
        n_fig += 1
        plt.subplot(331)
        plt.title(plot_title + ' sim_s 2')
        plt.plot(mo.time, mo.vb, color='red', linestyle='-', label='vb'+old_str)
        plt.plot(mo.time, mo.voc, color='black',  linestyle='--', label='voc'+old_str)
        plt.plot(mo.time, mo.voc_stat, color='blue', linestyle='-.', label='voc_stat'+old_str)
        if mo.vb_h is not None:
            plt.plot(mo.time, mo.voc_soc, color='orange', linestyle=':', label='voc_soc'+old_str)
            plt.plot(mo.time, mo.vb_h, color='cyan', linestyle=':', label='vb_hdwe'+old_str)
        plt.legend(loc=1)
        plt.subplot(332)
        plt.plot(so.time, so.vb_s, color='red', linestyle='-', label='vb_s'+old_str)
        plt.plot(so.time, so.voc_s, color='black',  linestyle='--', label='voc_s'+old_str)
        plt.plot(so.time, so.voc_stat_s, color='blue', linestyle='-.', label='voc_stat_s'+old_str)
        plt.legend(loc=1)
        plt.subplot(334)
        plt.plot(mv.time, mv.vb, color='red', linestyle='-', label='vb'+new_str)
        plt.plot(mv.time, mv.voc, color='black', linestyle='--', label='voc'+new_str)
        plt.plot(mv.time, mv.voc_stat, color='blue', linestyle='-.', label='voc_stat'+new_str)
        plt.legend(loc=1)
        plt.subplot(335)
        plt.plot(smv.time, smv.vb_s, color='red', linestyle='-', label='vb_s'+new_str)
        plt.plot(smv.time, smv.voc_s, color='black', linestyle='--', label='voc_s'+new_str)
        plt.plot(mv.time, mv.voc_soc, color='blue', linestyle='-.', label='voc_soc'+new_str)
        plt.plot(smv.time, smv.voc_stat_s, color='orange', linestyle=':', label='voc_stat_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(337)
        plt.plot(mo.time, mo.soc, color='red', linestyle='-', label='soc'+old_str)
        plt.plot(mo.time, mo.soc_ekf, color='black',  linestyle='--', label='soc_ekf'+old_str)
        plt.plot(mv.time, mv.soc, color='blue', linestyle='-.', label='soc'+new_str)
        plt.plot(mv.time, mv.soc_ekf, color='orange', linestyle=':', label='soc_ekf'+new_str)
        plt.legend(loc=1)
        plt.subplot(338)
        plt.plot(so.time, so.soc_s, color='orange', linestyle='-', label='soc_s'+old_str)
        plt.plot(smv.time, smv.soc_s, color='cyan', linestyle='--', label='soc_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(333)
        plt.plot(mo.time, mo.vb, color='red', linestyle='-', label='vb'+old_str)
        plt.plot(so.time, so.vb_s, color='black', linestyle='--',  label='vb_s'+old_str)
        plt.plot(mv.time, mv.vb, color='blue', linestyle='-.', label='vb'+new_str)
        plt.plot(smv.time, smv.vb_s, color='orange', linestyle=':', label='vb_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(336)
        plt.plot(mo.time, mo.voc, color='red', linestyle='-', label='voc'+old_str)
        plt.plot(mv.time, mv.voc, color='black', linestyle='--', label='voc'+new_str)
        plt.plot(so.time, so.voc_s, color='blue', linestyle='-.', label='voc_s'+old_str)
        plt.plot(smv.time, smv.voc_s, color='orange', linestyle=':', label='voc_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(339)
        plt.plot(mo.time, mo.voc_stat, color='red', linestyle='-', label='voc_stat'+old_str)
        plt.plot(so.time, so.voc_stat_s, color='black',  linestyle='--', label='voc_stat_s'+old_str)
        plt.plot(mv.time, mv.voc_stat, color='blue', linestyle='-.', label='voc_stat'+new_str)
        plt.plot(smv.time, smv.voc_stat_s, color='orange', linestyle=':', label='voc_stat_s'+new_str)
        plt.legend(loc=1)

        plt.figure()  # sim_s  3
        n_fig += 1
        plt.subplot(221)
        plt.title(plot_title + ' sim_s 3')
        plt.plot(mo.time, mo.soc, color='blue', linestyle='-', label='soc'+old_str)
        plt.plot(mv.time, mv.soc, color='red', linestyle='--', label='soc'+new_str)
        plt.plot(so.time, so.soc_s, color='green', linestyle='-.', label='soc_s'+old_str)
        if hasattr(sv, 'soc'):
            plt.plot(sv.time, sv.soc, color='orange', linestyle=':', label='sv.soc_s'+new_str)
        elif hasattr(sv, 'soc_s'):
            plt.plot(sv.time, sv.soc_s, color='orange', linestyle=':', label='sv.soc_s'+new_str)
        plt.plot(mo.time, mo.soc_ekf, color='cyan', linestyle='-', label='soc_ekf'+old_str)
        plt.plot(mv.time, mv.soc_ekf, color='magenta', linestyle='--', label='soc_ekf'+new_str)
        if hasattr(sv, 'soc'):
            plt.plot(sv.time, np.array(sv.soc)-.2, color='orange', linestyle='-', label='sv.soc_s'+new_str+'-0.2')
        elif hasattr(sv, 'soc_s'):
            plt.plot(sv.time, np.array(sv.soc_s)-.2, color='orange', linestyle='-', label='sv.soc_s'+new_str+'-0.2')
        plt.plot(smv.time, np.array(smv.soc_s)-.2, color='black', linestyle='--', label='smv.soc_s'+new_str+'-0.2')
        plt.legend(loc=1)
        plt.subplot(222)
        if mo.vb_h is not None:
            plt.plot(mo.soc, mo.vb_h, color='magenta', linestyle=':', label='vb_hdwe'+old_str)
        plt.plot(mo.soc, mo.voc_stat, color='cyan', linestyle=':', label='voc_stat'+old_str)
        if mo.voc_soc is not None:
            plt.plot(mo.soc, mo.voc_soc, color='blue', linestyle='-', label='voc_soc'+old_str)
        plt.plot(mv.soc, mv.voc_soc, color='red', linestyle='--', label='voc_soc'+new_str)
        plt.plot(so.soc_s, so.voc_stat_s, color='green', linestyle='-.', label='voc_stat_s'+old_str)
        if hasattr(sv, 'voc_stat'):
            plt.plot(sv.soc, sv.voc_stat, color='orange', linestyle=':', label='voc_stat_s'+new_str)
        elif hasattr(sv, 'voc_stat_s'):
            plt.plot(sv.soc_s, sv.voc_stat_s, color='orange', linestyle=':', label='voc_stat_s'+new_str)
        plt.plot(mo.soc, mo.vsat, color='red', linestyle='-', label='vsat'+old_str)
        plt.plot(mv.soc, mv.vsat, color='black', linestyle='--', label='vsat'+new_str)
        plt.plot(mv.soc, mv.voc_stat, color='orange', linestyle='--', label='voc_stat'+new_str)
        plt.legend(loc=1)
        plt.subplot(224)
        if mo.vb_h is not None:
            plt.plot(mo.time, mo.vb_h, color='magenta', linestyle=':', label='vb_hdwe'+old_str)
        plt.plot(mo.time, mo.voc_stat, color='cyan', linestyle=':', label='voc_stat'+old_str)
        if mo.voc_soc is not None:
            plt.plot(mo.time, mo.voc_soc, color='blue', linestyle='-', label='voc_soc'+old_str)
        plt.plot(mv.time, mv.voc_soc, color='red', linestyle='--', label='voc_soc'+new_str)
        plt.plot(so.time, so.voc_stat_s, color='green', linestyle='-.', label='voc_stat_s'+old_str)
        if hasattr(sv, 'voc_stat'):
            plt.plot(sv.time, sv.voc_stat, color='orange', linestyle=':', label='voc_stat_s'+new_str)
        elif hasattr(sv, 'voc_stat_s'):
            plt.plot(sv.time, sv.voc_stat_s, color='orange', linestyle=':', label='voc_stat_s'+new_str)
        plt.plot(mo.time, mo.vsat, color='red', linestyle='-', label='vsat'+old_str)
        plt.plot(mv.time, mv.vsat, color='black', linestyle='--', label='vsat'+new_str)
        plt.plot(mv.time, mv.voc_stat, color='orange', linestyle='-.', label='voc_stat'+new_str)
        plt.legend(loc=1)

        plt.figure()  # sim_s  4
        n_fig += 1
        plt.subplot(221)
        plt.title(plot_title + ' sim_s 4')
        plt.plot(mo.time, mo.soc, color='blue', linestyle='-', label='soc'+old_str)
        plt.plot(mv.time, mv.soc, color='red', linestyle='--', label='soc'+new_str)
        plt.plot(so.time, so.soc_s, color='green', linestyle='-.', label='soc_s'+old_str)
        if hasattr(sv, 'soc'):
            plt.plot(sv.time, sv.soc, color='orange', linestyle=':', label='sv.soc_s'+new_str)
        elif hasattr(sv, 'soc_s'):
            plt.plot(sv.time, sv.soc_s, color='orange', linestyle=':', label='sv.soc_s'+new_str)
        plt.plot(mo.time, mo.soc_ekf, color='cyan', linestyle='-', label='soc_ekf'+old_str)
        plt.plot(mv.time, mv.soc_ekf, color='magenta', linestyle='--', label='soc_ekf'+new_str)
        plt.legend(loc=1)
        plt.subplot(222)
        plt.plot(mo.soc, mo.voc_stat, color='blue', linestyle='-', label='voc_stat(soc) = z '+old_str)
        plt.plot(mv.soc, mv.voc_stat, color='red', linestyle='--', label='voc_stat(soc) = z '+new_str)
        plt.plot(mo.soc, mo.voc_ekf, color='green', linestyle='-.', label='voc_ekf(soc) = Hx '+old_str)
        if mo.voc_soc is not None:
            plt.plot(mo.soc, mo.voc_soc, color='green', linestyle=':', label='voc_soc'+old_str)
        plt.plot(mv.soc, mv.voc_soc, color='orange', linestyle='-', label='voc_soc'+new_str)
        if mo.vb_h is not None:
            plt.plot(mo.soc, mo.vb_h, color='blue', linestyle='-', label='vb'+old_str)
        if hasattr(sv, 'voc_stat'):
            plt.plot(sv.soc, sv.voc_stat, color='red', linestyle=':', label='voc_stat_s'+new_str)
        elif hasattr(sv, 'voc_stat_s'):
            plt.plot(sv.soc_s, sv.voc_stat_s, color='red', linestyle=':', label='voc_stat_s'+new_str)
        plt.ylim(12.5, 14.5)
        plt.legend(loc=1)
        plt.subplot(223)
        plt.plot(mo.soc, mo.voc_stat, color='magenta', linestyle='-', label='voc_stat(soc) = z'+old_str)
        plt.plot(mv.soc, mv.voc_stat, color='black', linestyle='--', label='voc_stat(soc) = z'+new_str)
        plt.plot(mv.soc, mv.voc_ekf, color='cyan', linestyle=':', label='voc_ekf(soc) = Hx'+new_str)
        if mo.vb_h is not None:
            plt.plot(mo.soc, mo.vb_h, color='blue', linestyle='-', label='vb'+old_str)
        if hasattr(sv, 'voc_stat'):
            plt.plot(sv.soc, sv.voc_stat, color='red', linestyle=':', label='voc_stat_s'+new_str)
        elif hasattr(sv, 'voc_stat_s'):
            plt.plot(sv.soc_s, sv.voc_stat_s, color='red', linestyle=':', label='voc_stat_s'+new_str)
        plt.ylim(12.5, 14.5)
        plt.legend(loc=1)
        plt.subplot(224)
        plt.plot(mo.time, mo.voc_stat, color='magenta', linestyle='-', label='voc_stat(soc) = z'+old_str)
        plt.plot(mo.time, mo.voc_ekf, color='cyan', linestyle='--', label='voc_ekf(soc) = Hx'+old_str)
        if mo.voc_soc is not None:
            plt.plot(mo.time, mo.voc_soc, color='green', linestyle='-.', label='voc_soc'+old_str)
        plt.plot(mv.time, mv.voc_soc, color='orange', linestyle=':', label='voc_soc'+new_str)
        if mo.vb_h is not None:
            plt.plot(mo.time, mo.vb_h, color='blue', linestyle='-', label='vb'+old_str)
        plt.plot(mv.time, mv.voc_stat, color='black', linestyle=':', label='voc_stat(soc) = z '+new_str)
        plt.plot(mv.time, mv.voc_ekf, color='cyan', linestyle=':', label='voc_ekf(soc) = Hx '+new_str)
        if hasattr(sv, 'voc_stat'):
            plt.plot(sv.time, sv.voc_stat, color='red', linestyle=':', label='voc_stat_s'+new_str)
        elif hasattr(sv, 'voc_stat_s'):
            plt.plot(sv.time, sv.voc_stat_s, color='red', linestyle=':', label='voc_stat_s'+new_str)
        plt.ylim(12.5, 14.5)
        plt.legend(loc=1)

        if mo.Fx is not None:  # ekf
            plt.figure()  # EKF  1
            n_fig += 1
            plt.subplot(331)
            plt.title(plot_title + ' EKF 1')
            plt.plot(mo.time, mo.u, color='blue', linestyle='-', label='u'+old_str)
            plt.plot(mv.time, mv.u_ekf, color='red', linestyle='--', label='u'+new_str)
            plt.legend(loc=1)
            plt.subplot(332)
            plt.plot(mo.time, mo.z, color='blue', linestyle='-', label='z'+old_str)
            plt.plot(mv.time, mv.z_ekf, color='red', linestyle='--', label='z'+new_str)
            plt.legend(loc=1)
            plt.subplot(333)
            plt.plot(smv.time, smv.reset_s, color='green', linestyle='--', label='reset_s'+new_str)
            plt.plot(so.time, so.sat_s, color='blue', linestyle='-.', label='sat_s'+old_str)
            plt.plot(smv.time, smv.sat_s, color='red', linestyle=':', label='sat_s'+new_str)
            plt.legend(loc=1)
            plt.subplot(334)
            plt.plot(mo.time, mo.Fx, color='blue', linestyle='-', label='Fx'+old_str)
            plt.plot(mv.time, mv.Fx, color='red', linestyle='--', label='Fx'+new_str)
            plt.legend(loc=1)
            plt.subplot(335)
            plt.plot(mo.time, mo.Bu, color='blue', linestyle='-', label='Bu'+old_str)
            plt.plot(mv.time, mv.Bu, color='red', linestyle='--', label='Bu'+new_str)
            plt.legend(loc=1)
            plt.subplot(336)
            plt.plot(mo.time, mo.Q, color='blue', linestyle='-', label='Q'+old_str)
            plt.plot(mv.time, mv.Q, color='red', linestyle='--', label='Q'+new_str)
            plt.legend(loc=1)
            plt.subplot(337)
            plt.plot(mo.time, mo.R, color='blue', linestyle='-', label='R'+old_str)
            plt.plot(mv.time, mv.R, color='red', linestyle='--', label='R'+new_str)
            plt.legend(loc=1)
            plt.subplot(338)
            plt.plot(mo.time, mo.P, color='blue', linestyle='-', label='P'+old_str)
            plt.plot(mv.time, mv.P, color='red', linestyle='--', label='P'+new_str)
            plt.legend(loc=1)
            plt.subplot(339)
            plt.plot(mo.time, mo.S, color='blue', linestyle='-', label='S'+old_str)
            plt.plot(mv.time, mv.S, color='red', linestyle='--', label='S'+new_str)
            plt.legend(loc=1)
            fig_file_name = filename + '_' + str(n_fig) + ".png"
            fig_files.append(fig_file_name)
            plt.savefig(fig_file_name, format="png")

            plt.figure()  # EKF  2
            n_fig += 1
            plt.subplot(331)
            plt.title(plot_title + ' EKF 2')
            plt.plot(mo.time, mo.K, color='blue', linestyle='-', label='K'+old_str)
            plt.plot(mv.time, mv.K, color='red', linestyle='--', label='K'+new_str)
            plt.legend(loc=1)
            plt.subplot(332)
            plt.plot(mo.time, mo.x, color='blue', linestyle='-', label='x'+old_str)
            plt.plot(mv.time, mv.x_ekf, color='red', linestyle='--', label='x'+new_str)
            plt.legend(loc=1)
            plt.subplot(333)
            plt.plot(mo.time, mo.y, color='blue', linestyle='-', label='y'+old_str)
            plt.plot(mv.time, mv.y_ekf, color='red', linestyle='--', label='y'+new_str)
            plt.legend(loc=1)
            plt.subplot(334)
            plt.plot(mo.time, mo.x_prior, color='blue', linestyle='-', label='x_prior'+old_str)
            plt.plot(mv.time, mv.x_prior, color='red', linestyle='--', label='x_prior'+new_str)
            plt.legend(loc=1)
            plt.subplot(335)
            plt.plot(mo.time, mo.P_prior, color='blue', linestyle='-', label='P_prior'+old_str)
            plt.plot(mv.time, mv.P_prior, color='red', linestyle='--', label='P_prior'+new_str)
            plt.legend(loc=1)
            plt.subplot(336)
            plt.plot(mo.time, mo.x_post, color='blue', linestyle='-', label='x_post'+old_str)
            plt.plot(mv.time, mv.x_post, color='red', linestyle='--', label='x_post'+new_str)
            plt.legend(loc=1)
            plt.subplot(337)
            plt.plot(mo.time, mo.P_post, color='blue', linestyle='-', label='P_post'+old_str)
            plt.plot(mv.time, mv.P_post, color='red', linestyle='--', label='P_post'+new_str)
            plt.legend(loc=1)
            plt.subplot(338)
            plt.plot(mo.time, mo.hx, color='blue', linestyle='-', label='hx'+old_str)
            plt.plot(mv.time, mv.hx, color='red', linestyle='--', label='hx'+new_str)
            plt.legend(loc=1)
            plt.subplot(339)
            plt.plot(mo.time, mo.H, color='blue', linestyle='-', label='H'+old_str)
            plt.plot(mv.time, mv.H, color='red', linestyle='--', label='H'+new_str)
            plt.legend(loc=1)
            fig_file_name = filename + '_' + str(n_fig) + ".png"
            fig_files.append(fig_file_name)
            plt.savefig(fig_file_name, format="png")

            plt.figure()  # EKF3
            n_fig += 1
            plt.subplot(221)
            plt.title(plot_title + ' EKF 3')
            plt.plot(mo.time, mo.ib, color='red', linestyle='-', label='ib' + old_str)
            plt.plot(mv.time, mv.ib, color='black', linestyle='--', label='ib' + new_str)
            plt.plot(mo.time, mo.u, color='cyan', linestyle='-.', label='u' + old_str)
            plt.plot(mv.time, mv.u_ekf, color='orange', linestyle=':', label='u' + new_str)
            plt.legend(loc=1)
            plt.subplot(222)
            plt.plot(mo.time, mo.vb, color='red', linestyle='-', label='vb'+old_str)
            plt.plot(mv.time, mv.vb, color='black', linestyle='--', label='vb'+new_str)
            plt.plot(mo.time, mo.voc_stat, color='cyan', linestyle='-.', label='z=voc_stat'+old_str)
            plt.plot(mv.time, mv.voc_stat, color='orange', linestyle=':', label='z=voc_stat'+new_str)
            plt.legend(loc=1)
            plt.subplot(223)
            plt.plot(mo.time, mo.soc, color='red', linestyle='-', label='soc'+old_str)
            plt.plot(mv.time, mv.soc, color='black', linestyle='--', label='soc'+new_str)
            plt.plot(mo.time, mo.soc_ekf, color='cyan',  linestyle='-.', label='x=soc_ekf'+old_str)
            plt.plot(mv.time, mv.soc_ekf, color='orange', linestyle=':', label='x=soc_ekf'+new_str)
            plt.legend(loc=1)
            plt.subplot(224)
            plt.plot(mo.time, mo.voc_stat, color='red', linestyle='-', label='z=voc_stat'+old_str)
            plt.plot(mv.time, mv.voc_stat, color='black', linestyle='--', label='z=voc_stat'+new_str)
            plt.plot(mo.time, mo.voc_ekf, color='cyan', linestyle='-.', label='voc_ekf(soc) = Hx'+old_str)
            plt.plot(mv.time, mv.voc_ekf, color='orange', linestyle=':', label='voc_ekf(soc) = Hx'+new_str)
            plt.legend(loc=1)

        if mo.voc_soc is not None:
            plt.figure()  # EKF  3
            n_fig += 1
            plt.subplot(111)
            plt.title(plot_title + ' EKF 3')
            plt.plot(mo.soc, mo.voc_stat, color='red', linestyle='-', label='voc_stat'+old_str)
            plt.plot(mo.soc, mo.voc_soc, color='black', linestyle=':', label='voc_soc'+old_str)
            plt.legend(loc=1)
            fig_file_name = filename + '_' + str(n_fig) + ".png"
            fig_files.append(fig_file_name)
            plt.savefig(fig_file_name, format="png")

            plt.figure()  # Hyst 1
            n_fig += 1
            plt.subplot(331)
            plt.title(plot_title + ' Hyst 1')
            # plt.plot(mo.time, mo.dv_hys_required, linestyle='-', color='black', label='dv_hys_required'+old_str)
            plt.plot(mo.time, -mo.e_wrap, linestyle='-', color='red', label='-e_wrap'+old_str)
            plt.plot(mv.time, (np.array(mv.voc_stat)-np.array(mv.voc_soc)), linestyle='--', color='blue', label='-e_wrap'+new_str)
            plt.plot(mo.time, mo.dv_hys, linestyle='-.', color='orange', label='dv_hys'+old_str)
            plt.plot(mv.time, mv.dv_hys, marker='.', markersize='1', markevery=48, linestyle='None', color='magenta', label='dv_hys'+new_str)
            plt.plot(so.time, so.dv_hys_s, linestyle=':', color='cyan', label='dv_hys_s'+old_str)
            if hasattr(sv, 'dv_hys'):
                plt.plot(sv.time, sv.dv_hys, marker='.', markersize='1', markevery=64, linestyle='None', color='black', label='dv_hys_s'+new_str)
            elif hasattr(sv, 'dv_hys_s'):
                plt.plot(sv.time, sv.dv_hys_s, marker='.', markersize='1', markevery=64, linestyle='None', color='black', label='dv_hys_s'+new_str)
            # plt.plot(mo.time, mo.dv_hys_chg, linestyle='-.', color='green', label='dv_hys_chg'+old_str)
            # plt.plot(mo.time, mo.dv_hys_dis, linestyle=':', color='red', label='dv_hys_dis'+old_str)
            plt.xlabel('sec')
            plt.legend(loc=4)
            plt.subplot(332)
            # plt.plot(mo.soc, mo.dv_hys_required, linestyle='-', color='black', label='dv_hys_required'+old_str)
            plt.plot(mo.soc, -mo.e_wrap, linestyle='--', color='red', label='-e_wrap'+old_str)
            plt.plot(mo.soc, mo.dv_hys, linestyle='-.', color='orange', label='dv_hys'+old_str)
            plt.plot(mv.soc, mv.dv_hys, marker='.', markersize='1', markevery=4, linestyle='None', color='magenta', label='dv_hys'+new_str)
            plt.plot(so.soc_s, so.dv_hys_s, linestyle=':', color='cyan', label='dv_hys_s'+old_str)
            if hasattr(sv, 'dv_hys'):
                plt.plot(sv.soc, sv.dv_hys, marker='.', markersize='1', markevery=5, linestyle='None', color='black', label='dv_hys_s'+new_str)
            elif hasattr(sv, 'dv_hys_s'):
                plt.plot(sv.soc_s, sv.dv_hys_s, marker='.', markersize='1', markevery=5, linestyle='None', color='black', label='dv_hys_s'+new_str)
            # plt.plot(mo.soc, mo.dv_hys_chg, linestyle='-.', color='green', label='dv_hys_chg'+old_str)
            # plt.plot(mo.soc, mo.dv_hys_dis, linestyle=':', color='red', label='dv_hys_dis'+old_str)
            plt.xlabel('soc')
            plt.legend(loc=4)
            plt.subplot(333)
            plt.plot(mo.time, mo.soc, linestyle='-', color='green', label='soc'+old_str)
            plt.plot(so.time, so.soc_s, linestyle='--', color='blue', label='soc_s'+old_str)
            plt.plot(mv.time, mv.soc, linestyle='-.', color='red', label='soc'+new_str)
            if hasattr(sv, 'soc'):
                plt.plot(sv.time, sv.soc, linestyle=':', color='cyan', label='soc_s'+new_str)
            elif hasattr(sv, 'soc_s'):
                plt.plot(sv.time, sv.soc_s, linestyle=':', color='cyan', label='soc_s'+new_str)
            plt.xlabel('sec')
            plt.legend(loc=4)
            plt.subplot(334)
            plt.plot(mo.time, mo.ib_sel, linestyle='-', color='black', label='ib_sel'+old_str)
            plt.plot(mo.time, mo.ioc, linestyle='--', color='cyan', label='ioc'+old_str)
            plt.xlabel('sec')
            plt.legend(loc=4)
            plt.subplot(335)
            plt.plot(mo.soc, mo.ib_sel, linestyle='-', color='black', label='ib_sel'+old_str)
            plt.plot(mo.soc, mo.ioc, linestyle='--', color='cyan', label='ioc'+old_str)
            plt.xlabel('soc')
            plt.legend(loc=4)
            plt.subplot(336)
            plt.plot(so.time, so.vb_s, color='black', linestyle='-', label='vb_s'+old_str)
            plt.plot(mo.time, mo.vb, color='orange', linestyle='--', label='vb'+old_str)
            plt.plot(so.time, so.voc_stat_s, color='magenta', linestyle='-', label='voc_stat_s'+old_str)
            plt.plot(mo.time, mo.voc_stat, color='pink', linestyle='--', label='voc_stat'+old_str)
            plt.plot(mo.time, mo.voc_soc, marker='.', markersize='1', markevery=32, linestyle='None', color='black', label='voc_soc'+old_str)
            plt.legend(loc=1)
            plt.subplot(337)
            plt.plot(mo.time, mo.vb, linestyle='-', color='green', label='vb'+old_str)
            plt.plot(mo.time, mo.voc, linestyle='--', color='red', label='voc'+old_str)
            plt.plot(mo.time, mo.voc_stat, linestyle='-.', color='pink', label='voc_stat'+old_str)
            plt.plot(mo.time, mo.voc_soc, marker='.', markersize='1', markevery=32, linestyle='None', color='black', label='voc_soc'+old_str)
            plt.xlabel('sec')
            plt.legend(loc=4)
            plt.subplot(338)
            plt.plot(mo.soc, mo.vb, linestyle='-', color='green', label='vb'+old_str)
            plt.plot(mo.soc, mo.voc, linestyle='--', color='red', label='voc'+old_str)
            plt.plot(mo.soc, mo.voc_stat, linestyle='-.', color='blue', label='voc_stat'+old_str)
            plt.plot(mo.soc, mo.voc_soc, linestyle=':', color='black', label='voc_soc'+old_str)
            plt.xlabel('soc')
            plt.legend(loc=4)
            plt.subplot(339)
            plt.plot(mo.time, mo.vb, color='green', linestyle='-', label='vb'+old_str)
            plt.plot(mv.time, mv.vb, color='orange', linestyle='--', label='vb'+new_str)
            plt.plot(mo.time, mo.voc, color='blue', linestyle='-.', label='voc'+old_str)
            plt.plot(mv.time, mv.voc, marker='+', markersize='3', markevery=4, linestyle='None', color='black', label='voc'+new_str)
            plt.plot(mo.time, mo.voc_stat, color='magenta', linestyle='-', label='voc_stat'+old_str)
            plt.plot(mv.time, mv.voc_stat, color='pink', linestyle='--', label='voc_stat'+new_str)
            plt.legend(loc=1)
            fig_file_name = filename + '_' + str(n_fig) + ".png"
            fig_files.append(fig_file_name)
            plt.savefig(fig_file_name, format="png")

        plt.figure()  # 7 off/on sim
        n_fig += 1
        plt.subplot(221)
        plt.title(plot_title + ' off/on sim 1')
        plt.plot(so.time, so.vb_s, color='black', linestyle='-', label='vb_s'+old_str)
        if hasattr(sv, 'vb'):
            plt.plot(sv.time, sv.vb, color='orange', linestyle=':', label='vb_s'+new_str)
        if hasattr(sv, 'vb_s'):
            plt.plot(sv.time, sv.vb_s, color='orange', linestyle=':', label='vb_s'+new_str)
        plt.plot(so.time, so.voc_s, color='blue', linestyle='--', label='voc_s'+old_str)
        if hasattr(sv, 'voc'):
            plt.plot(sv.time, sv.voc, color='red', linestyle='--', label='voc_s'+new_str)
        elif hasattr(sv, 'voc_s'):
            plt.plot(sv.time, sv.voc_s, color='red', linestyle='--', label='voc_s'+new_str)
        plt.plot(so.time, so.voc_stat_s, color='magenta', linestyle='-.', label='voc_stat_s'+old_str)
        if hasattr(sv, 'voc_stat'):
            plt.plot(sv.time, sv.voc_stat, color='green', linestyle=':', label='voc_stat_s'+new_str)
        elif hasattr(sv, 'voc_stat_s'):
            plt.plot(sv.time, sv.voc_stat_s, color='green', linestyle=':', label='voc_stat_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(222)
        plt.plot(so.time, so.dv_hys_s, linestyle='-', color='black', label='dv_hys_s'+old_str)
        if hasattr(sv, 'dv_hys'):
            plt.plot(sv.time, sv.dv_hys, linestyle='--', color='orange', label='dv_hys_s'+new_str)
        elif hasattr(sv, 'dv_hys_s'):
            plt.plot(sv.time, sv.dv_hys_s, linestyle='--', color='orange', label='dv_hys_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(223)
        plt.plot(so.time, so.soc_s, linestyle='-', color='black', label='soc_s'+old_str)
        if hasattr(sv, 'soc'):
            plt.plot(sv.time, sv.soc, linestyle='--', color='orange', label='soc_s'+new_str)
        elif hasattr(sv, 'soc_s'):
            plt.plot(sv.time, sv.soc_s, linestyle='--', color='orange', label='soc_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(224)
        plt.plot(so.time, so.ib_s, linestyle='-', color='black', label='ib_s'+old_str)
        if hasattr(sv, 'ib'):
            plt.plot(sv.time, sv.ib, linestyle='--', color='orange', label='ib_s'+new_str)
        if hasattr(sv, 'ib_s'):
            plt.plot(sv.time, sv.ib_s, linestyle='--', color='orange', label='ib_s'+new_str)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # 8 off/on mon
        n_fig += 1
        plt.subplot(221)
        plt.title(plot_title + ' off/on mon 1')
        plt.plot(mo.time, mo.vb, color='black', linestyle='-', label='vb'+old_str)
        plt.plot(mo.time, mo.voc, color='blue', linestyle='--', label='voc'+old_str)
        plt.plot(mo.time, mo.voc_stat, color='magenta', linestyle='-.', label='voc_stat'+old_str)
        plt.legend(loc=1)
        plt.subplot(222)
        plt.plot(mo.time, mo.dv_hys, linestyle='-', color='black', label='dv_hys'+old_str)
        plt.plot(mv.time, mv.dv_hys, linestyle='--', color='orange', label='dv_hys'+new_str)
        plt.legend(loc=1)
        plt.subplot(223)
        plt.plot(mo.time, mo.soc, linestyle='-', color='black', label='soc'+old_str)
        plt.plot(mv.time, mv.soc, linestyle='--', color='orange', label='soc'+new_str)
        plt.legend(loc=1)
        plt.subplot(224)
        plt.plot(mo.time, mo.ib_sel, linestyle='-', color='red', label='ib_sel'+old_str)
        plt.plot(so.time, so.ib_s, linestyle='--', color='cyan', label='ib_in_s'+old_str)
        plt.plot(mo.time, mo.ib_charge, linestyle='-.', color='blue', label='ib_charge'+old_str)
        plt.plot(mv.time, mv.ib_charge, linestyle=':', color='orange', label='ib_charge'+new_str)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

    plt.figure()  # GP 1
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title + ' GP 1')
    if so is not None:
        plt.plot(so.time, so.vb_s, color='black', linestyle='-', label='vb_s'+old_str)
    plt.plot(smv.time, smv.vb_s, color='orange', linestyle='--', label='vb_s'+new_str)
    if so is not None:
        plt.plot(so.time, so.voc_s, color='blue', linestyle='-.', label='voc_s'+old_str)
    plt.plot(smv.time, smv.voc_s, color='red', linestyle=':', label='voc_s'+new_str)
    if so is not None:
        plt.plot(so.time, so.voc_stat_s, color='magenta', linestyle='-.', label='voc_stat_s'+old_str)
    plt.plot(smv.time, smv.voc_stat_s, color='green', linestyle=':', label='voc_stat_s'+new_str)
    plt.legend(loc=1)
    plt.subplot(222)
    if so is not None:
        plt.plot(so.time, so.dv_hys_s, linestyle='-', color='black', label='dv_hys_s'+old_str)
    plt.plot(smv.time, smv.dv_hys_s, linestyle='--', color='orange', label='dv_hys_s'+new_str)
    plt.legend(loc=1)
    plt.subplot(223)
    if so is not None:
        plt.plot(so.time, so.soc_s, linestyle='-', color='black', label='soc_s'+old_str)
    plt.plot(smv.time, smv.soc_s, linestyle='--', color='orange', label='soc_s'+new_str)
    plt.legend(loc=1)
    plt.subplot(224)
    if so is not None:
        plt.plot(so.time, so.ib_in_s, linestyle='-', color='blue', label='ib_in_s'+old_str)
    if smv is not None:
        plt.plot(smv.time, smv.ib_in_s, linestyle='--', color='red', label='ib_in_s'+new_str)
    if hasattr(smv, 'ib_fut_s'):
        plt.plot(smv.time, smv.ib_fut_s, linestyle='-.', color='orange', label='ib_fut_s'+new_str)
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # GP 2
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title + ' GP 2')
    plt.plot(mo.time, mo.vb, color='black', linestyle='-', label='vb'+old_str)
    plt.plot(mv.time, mv.vb, color='orange', linestyle='--', label='vb'+new_str)
    plt.plot(mo.time, mo.voc, color='blue', linestyle='-.', label='voc'+old_str)
    plt.plot(mv.time, mv.voc, color='red', linestyle=':', label='voc'+new_str)
    plt.plot(mo.time, mo.voc_stat, color='cyan', linestyle='-.', label='voc_stat'+old_str)
    plt.plot(mv.time, mv.voc_stat, color='black', linestyle=':', label='voc_stat'+new_str)
    plt.legend(loc=1)
    plt.subplot(222)
    plt.plot(mo.time, mo.dv_hys, linestyle='-', color='black', label='dv_hys'+old_str)
    plt.plot(mv.time, mv.dv_hys, linestyle='--', color='orange', label='dv_hys'+new_str)
    plt.legend(loc=1)
    plt.subplot(223)
    plt.plot(mo.time, mo.soc, linestyle='-', color='black', label='soc'+old_str)
    plt.plot(mv.time, mv.soc, linestyle='--', color='orange', label='soc'+new_str)
    plt.legend(loc=1)
    plt.subplot(224)
    plt.plot(mo.time, mo.ib_sel, linestyle='-', color='black', label='ib_sel'+old_str)
    if so is not None:
        plt.plot(so.time, so.ib_s, linestyle='--', color='cyan', label='ib_in_s'+old_str)
    plt.plot(mv.time, mv.ib_charge, linestyle='-.', color='orange', label='ib_charge'+new_str)
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # GP 3 Tune
    n_fig += 1
    plt.subplot(331)
    plt.title(plot_title + ' GP 3 Tune')
    plt.plot(mo.time, mo.dv_dyn, color='blue', linestyle='-', label='dv_dyn'+old_str)
    plt.plot(mv.time, mv.dv_dyn, color='cyan', linestyle='--', label='dv_dyn'+new_str)
    plt.plot(mo.time, so.dv_dyn_s, color='black', linestyle='-.', label='dv_dyn_s'+old_str)
    plt.plot(mv.time, smv.dv_dyn_s, color='magenta', linestyle=':', label='dv_dyn_s'+new_str)
    plt.xlabel('sec')
    plt.legend(loc=3)
    plt.subplot(332)
    plt.plot(mo.time, mo.soc, linestyle='-', color='blue', label='soc'+old_str)
    plt.plot(mv.time, mv.soc, linestyle='--', color='cyan', label='soc'+new_str)
    plt.plot(so.time, so.soc_s, linestyle='-.', color='black', label='soc_s'+old_str)
    plt.plot(sv.time, smv.soc_s, linestyle=':', color='magenta', label='soc_s'+new_str)
    plt.plot(mo.time, mo.soc_ekf, linestyle='-', color='blue', label='soc_ekf'+old_str)
    plt.plot(mv.time, mv.soc_ekf, linestyle='--', color='red', label='soc_ekf'+new_str)
    plt.xlabel('sec')
    plt.legend(loc=4)
    plt.subplot(333)
    plt.plot(mo.time, mo.ib_sel, linestyle='-', color='blue', label='ib_sel'+old_str)
    plt.plot(mv.time, mv.ib_in, linestyle='-', color='cyan', label='ib_in'+new_str)
    plt.plot(so.time, so.ib_s, linestyle='--', color='black', label='ib_in_s'+old_str)
    plt.plot(smv.time, smv.ib_in_s, linestyle=':', color='red', label='ib_in_s'+new_str)
    plt.xlabel('sec')
    plt.legend(loc=3)
    plt.subplot(334)
    plt.plot(mo.time, mo.voc, linestyle='-', color='blue', label='voc'+old_str)
    plt.plot(mv.time, mv.voc, linestyle='--', color='cyan', label='voc'+new_str)
    plt.plot(mo.time, mo.voc_stat, linestyle='-.', color='blue', label='voc_stat'+old_str)
    plt.plot(mv.time, mv.voc_stat, linestyle=':', color='red', label='voc_stat'+new_str)
    plt.plot(mo.time, mo.vsat, linestyle='-', color='orange', label='vsat'+old_str)
    plt.plot(mv.time, mv.vsat, linestyle='--', color='pink', label='vsat'+new_str)
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(335)
    plt.plot(mo.time, mo.e_wrap, color='black', linestyle='-', label='e_wrap'+old_str)
    plt.plot(mv.time, mv.e_wrap, color='orange', linestyle='--', label='e_wrap'+new_str)
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(336)
    plt.plot(mo.soc, mo.vb, color='blue', linestyle='-', label='vb'+old_str)
    plt.plot(mv.soc, mv.vb, color='cyan', linestyle='--', label='vb'+new_str)
    plt.plot(so.soc_s, so.vb_s, color='black', linestyle='-.', label='vb_s'+old_str)
    plt.plot(smv.soc_s, smv.vb_s, color='magenta', linestyle=':', label='vb_s'+new_str)
    plt.xlabel('state-of-charge')
    plt.legend(loc=2)
    plt.subplot(337)
    plt.plot(mo.time, mo.vb, color='blue', linestyle='-', label='vb'+old_str)
    plt.plot(mv.time, mv.vb, color='cyan', linestyle='--', label='vb'+new_str)
    plt.plot(so.time, so.vb_s, color='black', linestyle='-.', label='vb_s'+old_str)
    plt.plot(smv.time, smv.vb_s, color='magenta', linestyle=':', label='vb_s'+new_str)
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(338)
    plt.plot(mo.time, mo.dv_hys, color='blue', linestyle='-', label='dv_hys'+old_str)
    plt.plot(mv.time, mv.dv_hys, color='cyan', linestyle='--', label='dv_hys'+new_str)
    plt.plot(so.time, so.dv_hys_s, color='black', linestyle='-.', label='dv_hys_s'+old_str)
    plt.plot(smv.time, smv.dv_hys_s, color='magenta', linestyle=':', label='dv_hys_s'+new_str)
    plt.xlabel('sec')
    plt.legend(loc=3)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files


def write_clean_file(txt_file, type_, title_key, unit_key, skip=1, path_to_data='.', path_to_temp='.',
                     comment_str='#'):
    data_file = path_to_data+'/'+txt_file
    csv_file = path_to_temp+'/'+txt_file.replace('.txt', type_ + '.csv', 1)
    # Header
    have_header_str = None
    with open(data_file, "r", encoding='cp437') as input_file:
        with open(csv_file, "w") as output:
            try:
                for line in input_file:
                    if line.__contains__(title_key):
                        if have_header_str is None:
                            have_header_str = True  # write one title only
                            output.write(line)
            except:
                print("DataOverModel381:", line)  # last line
    # Data
    num_lines = 0
    num_lines_in = 0
    num_skips = 0
    length = 0
    unit_key_found = False
    with open(data_file, "r", encoding='cp437') as input_file:  # reads all characters even bad ones
        with open(csv_file, "a") as output:
            for line in input_file:
                if line.__contains__(unit_key):
                    unit_key_found = True
                    if num_lines == 0:  # use first data line to screen out short and long lines that have key
                        length = line.count(",")
                    if line.count(",") == length and (num_lines == 0 or ((num_lines_in+1) % skip) == 0)\
                            and line.count(comment_str) == 0:
                        output.write(line)
                        num_lines += 1
                    else:
                        num_skips += 1
                    num_lines_in += 1
    if not num_lines:
        csv_file = None
        print("I(write_clean_file): no data to write")
        if not unit_key_found:
            print("W(write_clean_file):  unit_key not found in ", txt_file, ".  Looking with ", unit_key)
    else:
        print("Wrote(write_clean_file):", csv_file, num_lines, "lines", num_skips, "skips", length, "fields")
    return csv_file


class SavedData:
    def __init__(self, data=None, sel=None, ekf=None, time_end=None, zero_zero=False):
        if data is None:
            self.i = 0
            self.time = None
            self.time_min = None
            self.time_day = None
            self.dt = None  # Update time, s
            self.unit = None  # text title
            self.hm = None  # hours, minutes
            self.cTime = None  # Control time, s
            self.ib = None  # Bank current, A
            self.ioc = None  # Hys indicator current, A
            self.voc = None
            self.voc_soc = None
            # self.ib_past = None  # Past bank current, A
            self.ib_charge = None  # BMS switched current, A
            self.vb = None  # Bank voltage, V
            self.chm = None  # Battery chemistry code
            self.sat = None  # Indication that battery is saturated, T=saturated
            self.sel = None  # Current source selection, 0=amp, 1=no amp
            self.mod = None  # Configuration control code, 0=all hardware, 7=all simulated, +8 tweak test
            self.bms_off = None  # Battery management system off, T=off
            self.Tb = None  # Battery bank temperature, deg C
            self.vsat = None  # Monitor Bank saturation threshold at temperature, deg C
            self.dv_dyn = None  # Monitor Bank current induced back emf, V
            self.dv_hys = None  # Drop across hysteresis, V
            self.voc_stat = None  # Monitor Static bank open circuit voltage, V
            self.voc = None  # Bank VOC estimated from vb and RC model, V
            self.voc_ekf = None  # Monitor bank solved static open circuit voltage, V
            self.y_ekf = None  # Monitor single battery solver error, V
            self.soc_s = None  # Simulated state of charge, fraction
            self.soc_ekf = None  # Solved state of charge, fraction
            self.soc = None  # Coulomb Counter fraction of saturation charge (q_capacity_) available (0-1)
            self.time_ref = 0.  # Adjust time for start of ib input
        else:
            self.i = 0
            self.cTime = np.array(data.cTime)
            self.time = np.array(data.cTime)
            self.ib = np.array(data.ib)
            # manage data shape
            # Find first non-zero ib and use to adjust time
            # Ignore initial run of non-zero ib because resetting from previous run
            if zero_zero:
                self.zero_end = 0
            else:
                try:
                    zero_start = np.where(self.ib == 0.0)[0][0]
                    self.zero_end = zero_start
                    while self.ib[self.zero_end] == 0.0:  # stop after first non-zero
                        self.zero_end += 1
                    self.zero_end -= 1  # backup one
                except:
                    self.zero_end = 0
            self.time_ref = self.time[self.zero_end]
            self.time -= self.time_ref
            self.time_min = self.time / 60.
            self.time_day = self.time / 3600. / 24.

            # Truncate
            if time_end is None:
                i_end = len(self.time)
                if sel is not None:
                    self.c_time_s = np.array(sel.c_time) - self.time_ref
                    i_end = min(i_end, len(self.c_time_s))
                if ekf is not None:
                    self.c_time_s = np.array(ekf.c_time) - self.time_ref
                    i_end = min(i_end, len(self.c_time_s))
            else:
                i_end = np.where(self.time <= time_end)[0][-1] + 1
                if sel is not None:
                    self.c_time_s = np.array(sel.c_time) - self.time_ref
                    i_end_sel = np.where(self.c_time_s <= time_end)[0][-1] + 1
                    i_end = min(i_end, i_end_sel)
                    self.zero_end = min(self.zero_end, i_end-1)
                if ekf is not None:
                    self.c_time_s = np.array(ekf.c_time) - self.time_ref
                    i_end_ekf = np.where(self.c_time_s <= time_end)[0][-1] + 1
                    i_end = min(i_end, i_end_ekf)
                    self.zero_end = min(self.zero_end, i_end - 1)
            self.cTime = self.cTime[:i_end]
            self.dt = np.array(data.dt[:i_end])
            self.time = np.array(self.time[:i_end])
            self.ib = np.array(data.ib[:i_end])
            self.ioc = np.array(data.ioc[:i_end])
            self.voc_soc = np.array(data.voc_soc[:i_end])
            self.vb = np.array(data.vb[:i_end])
            self.chm = np.array(data.chm[:i_end])
            self.sat = np.array(data.sat[:i_end])
            self.sel = np.array(data.sel[:i_end])
            self.mod_data = np.array(data.mod[:i_end])
            self.bms_off = np.array(data.bmso[:i_end])
            # not_bms_off = self.bms_off < 1
            # bms_off_and_not_charging = self.bms_off * not_bms_off
            # self.ib_charge = self.ib * (bms_off_and_not_charging < 1)
            self.ib_charge = np.array(data.ib_charge[:i_end])
            self.Tb = np.array(data.Tb[:i_end])
            self.vsat = np.array(data.vsat[:i_end])
            self.dv_dyn = np.array(data.dv_dyn[:i_end])
            self.voc_stat = np.array(data.voc_stat[:i_end])
            self.voc = self.vb - self.dv_dyn
            self.dv_hys = self.voc - self.voc_stat
            self.voc_ekf = np.array(data.voc_ekf[:i_end])
            self.y_ekf = np.array(data.y_ekf[:i_end])
            self.soc_s = np.array(data.soc_s[:i_end])
            self.soc_ekf = np.array(data.soc_ekf[:i_end])
            self.soc = np.array(data.soc[:i_end])
        if sel is None:
            self.c_time_s = None
            self.res = None
            self.user_sel = None
            self.cc_dif = None
            self.ccd_fa = None
            self.ibmh = None
            self.ibnh = None
            self.ibmm = None
            self.ibnm = None
            self.ibm = None
            self.ib_diff = None
            self.ib_diff_f = None
            self.ib_diff_flt = None
            self.ib_diff_fa = None
            self.voc_soc_sel = None
            self.e_wrap = None
            self.e_wrap_filt = None
            self.wh_flt = None
            self.wl_flt = None
            self.red_loss = None
            self.wh_fa = None
            self.wl_fa = None
            self.wv_fa = None
            self.ib_sel_stat = None
            self.ib_h = None
            self.ib_s = None
            self.mib = None
            # self.ib = np.append(np.array(self.ib_past[1:]), self.ib_past[-1])  # shift time to present
            self.ib_sel = None
            self.vb_h = None
            self.vb_s = None
            self.mvb = None
            self.vb = self.vb
            self.Tb_h = None
            self.Tb_s = None
            self.mtb = None
            self.Tb_f = None
            self.vb_sel = None
            self.ib_rate = None
            self.ib_quiet = None
            self.dscn_flt = None
            self.dscn_fa = None
            self.vb_flt = None
            self.vb_fa = None
            self.ib_finj = None
            self.Tb_finj = None
            self.vb_finj = None
            self.tb_sel = None
            self.tb_flt = None
            self.tb_fa = None
            self.ccd_thr = None
            self.ewh_thr = None
            self.ewl_thr = None
            self.ibd_thr = None
            self.ibq_thr = None
            self.preserving = None
        else:
            falw = np.array(sel.falw[:i_end], dtype=np.uint16)
            fltw = np.array(sel.fltw[:i_end], dtype=np.uint16)
            self.c_time_s = np.array(sel.c_time[:i_end])
            self.res = np.array(sel.res[:i_end])
            self.user_sel = np.array(sel.user_sel[:i_end])
            self.cc_dif = np.array(sel.cc_dif[:i_end])
            self.ccd_fa = np.bool8(np.array(falw) & 2**4)
            self.ibmh = np.array(sel.ibmh[:i_end])
            self.ibnh = np.array(sel.ibnh[:i_end])
            self.ibmm = np.array(sel.ibmm[:i_end])
            self.ibnm = np.array(sel.ibnm[:i_end])
            self.ibm = np.array(sel.ibm[:i_end])
            self.ib_diff = np.array(sel.ib_diff[:i_end])
            self.ib_diff_f = np.array(sel.ib_diff_f[:i_end])
            self.ib_diff_flt = np.bool8((np.array(fltw) & 2**8) | (np.array(fltw) & 2**9))
            self.ib_diff_fa = np.bool8((np.array(falw) & 2**8) | (np.array(falw) & 2**9))
            self.voc_soc_sel = np.array(sel.voc_soc[:i_end])
            self.e_wrap = np.array(sel.e_w[:i_end])
            self.e_wrap_filt = np.array(sel.e_w_f[:i_end])
            self.wh_flt = np.bool8(np.array(fltw) & 2**5)
            self.wl_flt = np.bool8(np.array(fltw) & 2**6)
            self.red_loss = np.bool8(np.array(fltw) & 2**7)
            self.wh_fa = np.bool8(np.array(falw) & 2**5)
            self.wl_fa = np.bool8(np.array(falw) & 2**6)
            self.wv_fa = np.bool8(np.array(falw) & 2**7)
            self.ib_sel_stat = np.array(sel.ib_sel_stat[:i_end])
            self.ib_h = np.array(sel.ib_h[:i_end])
            self.ib_s = np.array(sel.ib_s[:i_end])
            self.mib = np.array(sel.mib[:i_end])
            self.ib_sel = np.array(sel.ib[:i_end])
            self.vb_h = np.array(sel.vb_h[:i_end])
            self.vb_s = np.array(sel.vb_s[:i_end])
            self.mvb = np.array(sel.mvb[:i_end])
            self.vb = np.array(sel.vb[:i_end])
            self.Tb_h = np.array(sel.Tb_h[:i_end])
            self.Tb_s = np.array(sel.Tb_s[:i_end])
            self.mtb = np.array(sel.mtb[:i_end])
            self.Tb_f = np.array(sel.Tb_f[:i_end])
            self.vb_sel = np.array(sel.vb_sel[:i_end])
            self.ib_rate = np.array(sel.ib_rate[:i_end])
            self.ib_quiet = np.array(sel.ib_quiet[:i_end])
            self.dscn_flt = np.bool8(np.array(fltw) & 2**10)
            self.dscn_fa = np.bool8(np.array(falw) & 2**10)
            self.vb_flt = np.bool8(np.array(fltw) & 2**1)
            self.vb_fa = np.bool8(np.array(falw) & 2**1)
            self.tb_sel = np.array(sel.tb_sel[:i_end])
            self.tb_flt = np.bool8(np.array(fltw) & 2**0)
            self.tb_fa = np.bool8(np.array(falw) & 2**0)
            self.ccd_thr = np.array(sel.ccd_thr[:i_end])
            self.ewh_thr = np.array(sel.ewh_thr[:i_end])
            self.ewl_thr = np.array(sel.ewl_thr[:i_end])
            self.ibd_thr = np.array(sel.ibd_thr[:i_end])
            self.ibq_thr = np.array(sel.ibq_thr[:i_end])
            self.preserving = np.array(sel.preserving[:i_end])
        if ekf is None:
            self.c_time_e = None
            self.Fx = None
            self.Bu = None
            self.Q = None
            self.R = None
            self.P = None
            self.S = None
            self.K = None
            self.u = None
            self.x = None
            self.y = None
            self.z = None
            self.x_prior = None
            self.P_prior = None
            self.x_post = None
            self.P_post = None
            self.hx = None
            self.H = None
        else:
            self.c_time_s = np.array(ekf.c_time[:i_end])
            self.Fx = np.array(ekf.Fx_[:i_end])
            self.Bu = np.array(ekf.Bu_[:i_end])
            self.Q = np.array(ekf.Q_[:i_end])
            self.R = np.array(ekf.R_[:i_end])
            self.P = np.array(ekf.P_[:i_end])
            self.S = np.array(ekf.S_[:i_end])
            self.K = np.array(ekf.K_[:i_end])
            self.u = np.array(ekf.u_[:i_end])
            self.x = np.array(ekf.x_[:i_end])
            self.y = np.array(ekf.y_[:i_end])
            self.z = np.array(ekf.z_[:i_end])
            self.x_prior = np.array(ekf.x_prior_[:i_end])
            self.P_prior = np.array(ekf.P_prior_[:i_end])
            self.x_post = np.array(ekf.x_post_[:i_end])
            self.P_post = np.array(ekf.P_post_[:i_end])
            self.hx = np.array(ekf.hx_[:i_end])
            self.H = np.array(ekf.H_[:i_end])

    def __str__(self):
        s = "{},".format(self.unit[self.i])
        s += "{},".format(self.hm[self.i])
        s += "{:13.3f},".format(self.cTime[self.i])
        # s += "{},".format(self.T[self.i])
        s += "{:8.3f},".format(self.ib[self.i])
        s += "{:7.2f},".format(self.vsat[self.i])
        s += "{:5.2f},".format(self.dv_dyn[self.i])
        s += "{:5.2f},".format(self.voc_stat[self.i])
        s += "{:5.2f},".format(self.voc_ekf[self.i])
        s += "{:10.6f},".format(self.y_ekf[self.i])
        s += "{:7.3f},".format(self.soc_s[self.i])
        s += "{:5.3f},".format(self.soc_ekf[self.i])
        s += "{:5.3f},".format(self.soc[self.i])
        return s

    def mod(self):
        return self.mod_data[self.zero_end]


class SavedDataSim:
    def __init__(self, time_ref, data=None, time_end=None):
        if data is None:
            self.i = 0
            self.time = None
            self.time_min = None
            self.time_day = None
            self.unit = None  # text title
            self.c_time = None  # Control time, s
            self.chm_s = None
            self.bms_off_s = None
            self.Tb_s = None
            self.Tbl_s = None
            self.vsat_s = None
            self.voc_s = None
            self.voc_stat_s = None
            self.dv_dyn_s = None
            self.dv_hys_s = None
            self.vb_s = None
            self.ib_in_s = None
            self.ib_charge_s = None
            self.ioc_s = None
            self.ib_s = None
            self.sat_s = None
            self.dq_s = None
            self.soc_s = None
            self.reset_s = None
        else:
            self.i = 0
            self.c_time = np.array(data.c_time)
            self.time = np.array(data.c_time) - time_ref
            # Truncate
            if time_end is None:
                i_end = len(self.time)
            else:
                i_end = np.where(self.time <= time_end)[0][-1] + 1
            self.c_time = self.c_time[:i_end]
            self.time = self.time[:i_end]
            self.time_min = self.time / 60.
            self.time_day = self.time / 3600. / 24.
            self.chm_s = data.chm_s[:i_end]
            self.bms_off_s = data.bmso_s[:i_end]
            self.Tb_s = data.Tb_s[:i_end]
            self.Tbl_s = data.Tbl_s[:i_end]
            self.vb_s = data.vb_s[:i_end]
            self.vsat_s = data.vsat_s[:i_end]
            self.voc_stat_s = data.voc_stat_s[:i_end]
            self.dv_dyn_s = data.dv_dyn_s[:i_end]
            self.voc_s = self.vb_s - self.dv_dyn_s
            self.dv_hys_s = self.voc_s - self.voc_stat_s
            self.ib_s = data.ib_s[:i_end]
            self.ib_in_s = data.ib_in_s[:i_end]
            self.ib_charge_s = data.ib_charge_s[:i_end]
            self.ioc_s = data.ioc_s[:i_end]
            self.sat_s = data.sat_s[:i_end]
            self.dq_s = data.dq_s[:i_end]
            self.soc_s = data.soc_s[:i_end]
            self.reset_s = data.reset_s[:i_end]

    def __str__(self):
        s = "{},".format(self.unit[self.i])
        s += "{:13.3f},".format(self.c_time[self.i])
        s += "{:5.2f},".format(self.Tb_s[self.i])
        s += "{:5.2f},".format(self.Tbl_s[self.i])
        s += "{:5.2f},".format(self.Tbl_s[self.i])
        s += "{:8.3f},".format(self.vsat_s[self.i])
        s += "{:5.2f},".format(self.voc_stat_s[self.i])
        s += "{:5.2f},".format(self.dv_dyn_s[self.i])
        s += "{:5.2f},".format(self.vb_s[self.i])
        s += "{:8.3f},".format(self.ib_s[self.i])
        s += "{:7.3f},".format(self.sat_s[self.i])
        # s += "{:5.3f},".format(self.ddq_s[self.i])
        s += "{:5.3f},".format(self.dq_s[self.i])
        # s += "{:5.3f},".format(self.qcap_s[self.i])
        s += "{:7.3f},".format(self.soc_s[self.i])
        s += "{:d},".format(self.reset_s[self.i])
        return s


if __name__ == '__main__':
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    plt.rcParams['axes.grid'] = True

    def compare_print(mo, mv):
        s = " time,      ib,                   vb,              dv_dyn,          voc_stat,\
                    voc,        voc_ekf,         y_ekf,               soc_ekf,      soc,\n"
        for i in range(len(mv.time)):
            s += "{:7.3f},".format(mo.time[i])
            s += "{:11.3f},".format(mo.ib[i])
            s += "{:9.3f},".format(mv.ib[i])
            s += "{:9.2f},".format(mo.vb[i])
            s += "{:5.2f},".format(mv.vb[i])
            s += "{:9.2f},".format(mo.dv_dyn[i])
            s += "{:5.2f},".format(mv.dv_dyn[i])
            s += "{:9.2f},".format(mo.voc_stat[i])
            s += "{:5.2f},".format(mv.voc_stat[i])
            s += "{:9.2f},".format(mo.voc[i])
            s += "{:5.2f},".format(mv.voc[i])
            s += "{:9.2f},".format(mo.voc_ekf[i])
            s += "{:5.2f},".format(mv.voc_ekf[i])
            s += "{:13.6f},".format(mo.y_ekf[i])
            s += "{:9.6f},".format(mv.y_ekf[i])
            s += "{:7.3f},".format(mo.soc_ekf[i])
            s += "{:5.3f},".format(mv.soc_ekf[i])
            s += "{:7.3f},".format(mo.soc[i])
            s += "{:5.3f},".format(mv.soc[i])
            s += "\n"
        return s


    def main(data_file_old_txt, unit_key):
        # Trade study inputs
        # i-->0 provides continuous anchor to reset filter (why?)  i shifts important --> 2 current sensors,
        #   hyst in ekf
        # saturation provides periodic anchor to reset filter
        # reset soc periodically anchor user display
        # tau_sd creating an anchor.   So large it's just a pass through
        # TODO:  temp sensitivities and mitigation

        # Config inputs
        # from MonSimNomConfig import *

        # Transient  inputs
        time_end = None
        # time_end = 1500.

        # Load data (must end in .txt) txt_file, type, title_key, unit_key
        data_file_clean = write_clean_file(data_file_old_txt, type_='_mon', title_key='unit,',
                                           unit_key=unit_key)
        data_file_sim_clean = write_clean_file(data_file_old_txt, type_='_sim', title_key='unit_m',
                                               unit_key='unit_sim,')

        # Load
        cols = ('unit', 'hm', 'cTime', 'dt', 'chm', 'sat', 'sel', 'mod', 'Tb', 'vb', 'ib', 'vsat', 'dv_dyn',
                'voc_stat', 'voc_ekf', 'y_ekf', 'soc_s', 'soc_ekf', 'soc')
        mon_old_raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols, dtype=None,
                                    encoding=None).view(np.recarray)
        mon_old = SavedData(mon_old_raw, time_end)
        cols_sim = ('unit_m', 'c_time', 'chm_s', 'bmso_s', 'Tb_s', 'Tbl_s', 'vsat_s', 'voc_stat_s', 'dv_dyn_s', 'vb_s',
                    'ib_s', 'ib_in_s', 'sat_s', 'dq_s', 'soc_s', 'reset_s')
        try:
            sim_old_raw = np.genfromtxt(data_file_sim_clean, delimiter=',', names=True, usecols=cols_sim,
                                        dtype=None, encoding=None).view(np.recarray)
            sim_old = SavedDataSim(mon_old.time_ref, sim_old_raw, time_end)
        except IOError:
            sim_old = None

        # Run model
        mon_ver, sim_ver, randles_ver, sim_s_ver = replicate(mon_old, init_time=1.)
        date_ = datetime.now().strftime("%y%m%d")
        mon_file_save = data_file_clean.replace(".csv", "_rep.csv")
        save_clean_file(mon_ver, mon_file_save, '_mon_rep' + date_)
        if data_file_sim_clean:
            sim_file_save = data_file_sim_clean.replace(".csv", "_rep.csv")
            save_clean_file_sim(sim_s_ver, sim_file_save, '_sim_rep' + date_)

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        data_root = data_file_clean.split('/')[-1].replace('.csv', '-')
        # if platform == 'linux':
        #     filename = "Python/Figures/" + data_root + sys.argv[0].split('/')[-1]
        # else:
        #     filename = data_root + sys.argv[0].split('/')[-1]
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        n_fig, fig_files = overall_batt(mon_ver, sim_ver, randles_ver, filename, fig_files, plot_title=plot_title,
                                        n_fig=n_fig, suffix='_ver')  # Could be confusing because sim over mon
        n_fig, fig_files = overall(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                   plot_title=plot_title, n_fig=n_fig, old_str='', new_str='_ver')
        # if platform != 'linux':
        #     unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf',
        #                             pathToSavePdfTo='../dataReduction/figures')
        #     cleanup_fig_files(fig_files)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf',
                                pathToSavePdfTo='../dataReduction/figures')
        cleanup_fig_files(fig_files)

        plt.show()


    # python DataOverModel.py("../dataReduction/rapidTweakRegressionTest20220711.txt", "pro_2022")

    """
    PyCharm Sample Run Configuration Parameters (right click in pyCharm - Modify Run Configuration:
        "../dataReduction/slowTweakRegressionTest20220711.txt" "pro_2022"
        "../dataReduction/serial_20220624_095543.txt"    "pro_2022"
        "../dataReduction/real world rapid 20220713.txt" "soc0_2022"
        "../dataReduction/real world Xp20 20220715.txt" "soc0_2022"
    
    PyCharm Terminal:
    python DataOverModel.py "../dataReduction/serial_20220624_095543.txt" "pro_2022"
    python DataOverModel.py "../dataReduction/ampHiFail20220731.txt" "pro_2022"
    

    android:
    python Python/DataOverModel.py "USBTerminal/serial_20220624_095543.txt" "pro_2022"
    """

    if __name__ == "__main__":
        import sys
        print(sys.argv[1:])
        main(sys.argv[1], sys.argv[2])
