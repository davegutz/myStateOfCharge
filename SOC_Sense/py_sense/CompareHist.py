# MonSim:  Monitor and Simulator replication of Particle Photon Application
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

""" Slice and dice the history dumps."""

from pyDAGx import myTables
import numpy as np
import numpy.lib.recfunctions as rf
import matplotlib.pyplot as plt
from resample import resample
from MonSim import replicate
from Battery import overall_batt
from Util import cat
from CompareFault import filter_Tb, add_stuff_f, add_stuff

UNIT_CAP_RATED = 108.4  # A-hr capacity of test article
IB_BAND = 1.  # Threshold to declare charging or discharging
TB_BAND = 15.  # Band around temperature to group data and correct

#  Rescale parameters design.  Minimal tuning attempt
#  This didn't work because low soc response of original design is too slow
VOC_RESET_05 = 0.  # Attempt to rescale to match voc_soc to all data
VOC_RESET_11 = 0.  # Attempt to rescale to match voc_soc to all data
VOC_RESET_20 = 0.  # Attempt to rescale to match voc_soc to all data
VOC_RESET_30 = -0.03  # Attempt to rescale to match voc_soc to all data
VOC_RESET_40 = 0.  # Attempt to rescale to match voc_soc to all data


def over_easy(hi, filename, mv_fast=None, mv_slow=None, fig_files=None, plot_title=None, fig_list=None, subtitle=None,
              x_sch=None, z_sch=None, voc_reset=0.):
    if fig_files is None:
        fig_files = []
    # Markers
    #
    # light symbols
    # '.' point
    # ',' pixel
    # '1' tri down
    # '2' tri up
    # '3' tri left
    # '4' tri right
    # '+' plus
    # 'x' x
    # '|' vline
    # '_' hline
    # 0 (TICKLEFT) tickleft
    # 1 (TICKRIGHT) tickright
    # 2 (TICKUP) tickup
    # 3 (TICKDOWN) tickdown
    #
    # bold filled symbols
    # 'o' circle
    # 'v' triangle down
    # '^' triangle up
    # '<' triangle left
    # '>' triangle right
    # '8' octagon
    # 's' square
    # 'p' pentagon
    # 'P' filled plus
    # '*' star
    # 'h' hexagon1
    # 'H' hexagon2
    # 4 (CARETLEFT) caretleft
    # 5 (CARETRIGHT) caretright
    # 6 (CARETUP) caretup
    # 7 (CARETDOWN) caretdown
    # 8 (CARETLEFTBASE) caretleft centered at base
    # 9 (CARETRIGHTBASE) caretright centered at base
    # 10 (CARETUPBASE) caretup centered at base
    # 11 (CARETDOWNBASE) caretdown centered at base

    # Colors

    plt.figure()  # 1
    fig_list += 1
    plt.subplot(331)
    plt.title(plot_title + ' h1')
    plt.suptitle(subtitle)
    plt.plot(hi.time_day, hi.soc, marker='.', markersize='3', linestyle='-', color='black', label='soc')
    plt.plot(hi.time_day, hi.soc_ekf, marker='+', markersize='3', linestyle='--', color='blue', label='soc_ekf')
    plt.plot(mv_fast.time_day-mv_fast.time_day[0], mv_fast.soc, linestyle='-.', color='red', label='soc_ver_300new')
    plt.plot(mv_fast.time_day-mv_fast.time_day[0], mv_fast.soc_ekf, linestyle=':', color='green', label='soc_ekf_ver_300new')
    plt.plot(mv_slow.time_day-mv_slow.time_day[0], mv_slow.soc, linestyle='--', color='magenta', label='soc_ver_300old')
    plt.plot(mv_slow.time_day-mv_slow.time_day[0], mv_slow.soc_ekf, linestyle='-.', color='cyan', label='soc_ekf_ver_300old')
    plt.legend(loc=1)
    plt.subplot(332)
    plt.plot(hi.time_day, hi.Tb, marker='.', markersize='3', linestyle='-', color='black', label='Tb')
    plt.legend(loc=1)
    plt.subplot(333)
    plt.plot(hi.time_day, hi.ib, marker='+', markersize='3', linestyle='-', color='green', label='ib')
    plt.legend(loc=1)
    plt.legend(loc=1)
    plt.subplot(335)
    plt.plot(hi.time_day, hi.falw, marker='+', markersize='3', linestyle='None', color='magenta', label='falw')
    plt.legend(loc=0)
    plt.subplot(336)
    plt.plot(hi.soc, hi.soc_ekf, marker='+', markersize='3', linestyle='-', color='blue', label='soc_ekf')
    plt.plot([0, 1], [0.2, 1.2], linestyle=':', color='red')
    plt.plot([0, 1], [-0.2, 0.8], linestyle=':', color='red')
    plt.xlim(0, 1)
    plt.ylim(0, 1)
    plt.xlabel('soc')
    plt.legend(loc=4)
    plt.subplot(337)
    plt.plot(hi.time_day, hi.dv_hys, marker='o', markersize='3', linestyle='-', color='blue', label='dv_hys')
    plt.plot(hi.time_day, hi.dv_hys_rescaled, marker='o', markersize='3', linestyle='-', color='cyan', label='dv_hys_rescaled')
    plt.plot(hi.time_day, hi.dv_hys_required, linestyle='--', color='black', label='dv_hys_required')
    plt.plot(hi.time_day, -hi.e_wrap, marker='o', markersize='3', linestyle='None', color='red', label='-e_wrap')
    plt.plot(hi.time_day, hi.dv_hys_remodel, marker='x', markersize='3', linestyle=':', color='lawngreen', label='dv_hys_remodel')
    plt.plot(hi.time_day, hi.dv_hys_redesign_chg, marker=3, markersize='3', linestyle=':', color='springgreen', label='dv_hys_redesign_chg')
    plt.plot(hi.time_day, hi.dv_hys_redesign_dis, marker=3, markersize='3', linestyle=':', color='orangered', label='dv_hys_redesign_dis')
    plt.xlabel('days')
    plt.legend(loc=1)
    plt.subplot(338)
    plt.plot(hi.time_day, hi.e_wrap, marker='o', markersize='3', linestyle='-', color='black', label='e_wrap')
    plt.plot(hi.time_day, hi.wv_fa, marker=0, markersize='4', linestyle=':', color='red', label='wrap_vb_fa')
    plt.plot(hi.time_day, hi.wl_fa-1, marker=2, markersize='4', linestyle=':', color='orange', label='wrap_lo_fa-1')
    plt.plot(hi.time_day, hi.wh_fa+1, marker=3, markersize='4', linestyle=':', color='green', label='wrap_hi_fa+1')
    plt.xlabel('days')
    plt.legend(loc=1)
    plt.subplot(339)
    plt.plot(hi.time_day, hi.vb, marker='.', markersize='3', linestyle='None', color='red', label='vb')
    plt.plot(hi.time_day, hi.voc_dyn, marker='.', markersize='3', linestyle='None', color='blue', label='voc_dyn')
    plt.plot(hi.time_day, hi.voc_stat_chg, marker='.', markersize='3', linestyle='None', color='green', label='voc_stat_chg')
    plt.plot(hi.time_day, hi.voc_stat_dis, marker='.', markersize='3', linestyle='None', color='red', label='voc_stat_dis')
    plt.xlabel('days')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 2
    fig_list += 1
    plt.subplot(221)
    plt.title(plot_title + ' h2')
    plt.suptitle(subtitle)
    plt.plot(hi.time_day, hi.vsat, marker='.', markersize='1', linestyle='-', color='orange', linewidth='1', label='vsat')
    plt.plot(hi.time_day, hi.vb, marker='1', markersize='3', linestyle='None', color='black', label='vb')
    plt.plot(hi.time_day, hi.voc_dyn, marker='.', markersize='3', linestyle='None', color='orange', label='voc_dyn')
    plt.plot(hi.time_day, hi.voc_stat_chg, marker='.', markersize='3', linestyle='-', color='green', label='voc_stat_chg')
    plt.plot(hi.time_day, hi.voc_stat_dis, marker='.', markersize='3', linestyle='-', color='red', label='voc_stat_dis')
    plt.plot(hi.time_day, hi.voc_soc, marker='2', markersize='3', linestyle=':', color='cyan', label='voc_soc')
    plt.xlabel('days')
    plt.legend(loc=1)
    plt.subplot(122)
    plt.plot(hi.time_day, hi.bms_off + 22, marker='h', markersize='3', linestyle='-', color='blue', label='bms_off+22')
    plt.plot(hi.time_day, hi.sat + 20, marker='s', markersize='3', linestyle='-', color='red', label='sat+20')
    plt.plot(hi.time_day, hi.dscn_fa + 18, marker='o', markersize='3', linestyle='-', color='black', label='dscn_fa+18')
    plt.plot(hi.time_day, hi.ib_diff_fa + 16, marker='^', markersize='3', linestyle='-', color='blue', label='ib_diff_fa+16')
    plt.plot(hi.time_day, hi.wv_fa + 14, marker='s', markersize='3', linestyle='-', color='cyan', label='wrap_vb_fa+14')
    plt.plot(hi.time_day, hi.wl_fa + 12, marker='p', markersize='3', linestyle='-', color='orange', label='wrap_lo_fa+12')
    plt.plot(hi.time_day, hi.wh_fa + 10, marker='h', markersize='3', linestyle='-', color='green', label='wrap_hi_fa+10')
    plt.plot(hi.time_day, hi.ccd_fa + 8, marker='H', markersize='3', linestyle='-', color='blue', label='cc_diff_fa+8')
    plt.plot(hi.time_day, hi.ib_noa_fa + 6, marker='+', markersize='3', linestyle='-', color='red', label='ib_noa_fa+6')
    plt.plot(hi.time_day, hi.ib_amp_fa + 4, marker='_', markersize='3', linestyle='-', color='magenta', label='ib_amp_fa+4')
    plt.plot(hi.time_day, hi.vb_fa + 2, marker='1', markersize='3', linestyle='-', color='cyan', label='vb_fa+2')
    plt.plot(hi.time_day, hi.tb_fa, marker='2', markersize='3', linestyle='-', color='orange', label='tb_fa')
    plt.ylim(-1, 24)
    plt.xlabel('days')
    plt.legend(loc=1)
    plt.subplot(223)
    plt.plot(hi.time_day, hi.ib, marker='.', markersize='3', linestyle='-', color='red', label='ib')
    plt.xlabel('days')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 3
    fig_list += 1
    plt.subplot(131)
    plt.title(plot_title + ' h3')
    plt.suptitle(subtitle)
    plt.plot(hi.soc_r, hi.voc_stat_r_dis, marker='o', markersize='3', linestyle='-', color='orange', label='voc_stat_r_dis')
    plt.plot(hi.soc_r, hi.voc_stat_r_chg, marker='o', markersize='3', linestyle='-', color='cyan', label='voc_stat_r_chg')
    plt.plot(hi.soc, hi.voc_stat_dis, marker='.', markersize='3', linestyle='None', color='red', label='voc_stat_dis')
    plt.plot(hi.soc, hi.voc_stat_chg, marker='.', markersize='3', linestyle='None', color='green', label='voc_stat_chg')
    plt.plot(x_sch, z_sch, marker='+', markersize='2', linestyle='--', color='black', label='Schedule')
    plt.xlabel('soc')
    plt.legend(loc=4)
    plt.ylim(12, 13.5)
    plt.subplot(132)
    plt.plot(hi.soc_r, hi.voc_stat_rescaled_r_dis, marker='o', markersize='3', linestyle='-', color='orangered', label='voc_stat_rescaled_r_dis')
    plt.plot(hi.soc_r, hi.voc_stat_rescaled_r_chg, marker='o', markersize='3', linestyle='-', color='springgreen', label='voc_stat_rescaled_r_chg')
    plt.plot(x_sch, z_sch+voc_reset, marker='+', markersize='2', linestyle='--', color='black', label='Schedule RESET')
    plt.xlabel('soc_r')
    plt.legend(loc=4)
    plt.ylim(12, 13.5)
    plt.subplot(133)
    plt.plot(hi.soc_r, hi.voc_stat_redesign_r_dis, marker='x', markersize='3', linestyle='-', color='red', label='voc_stat_redesign_r_dis')
    plt.plot(hi.soc_r, hi.voc_stat_redesign_r_chg, marker='x', markersize='3', linestyle='-', color='green', label='voc_stat_redesign_r_chg')
    plt.plot(x_sch, z_sch+voc_reset, marker='+', markersize='2', linestyle='--', color='black', label='Schedule RESET')
    plt.xlabel('soc_r')
    plt.legend(loc=4)
    plt.ylim(12, 13.5)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 4
    fig_list += 1
    plt.subplot(221)
    plt.title(plot_title + ' h4')
    plt.suptitle(subtitle)
    plt.plot(hi.time_day, hi.dv_hys, marker='o', markersize='3', linestyle='-', color='blue', label='dv_hys')
    plt.plot(hi.time_day, hi.dv_hys_rescaled, marker='o', markersize='3', linestyle='-', color='cyan', label='dv_hys_rescaled')
    plt.plot(hi.time_day, hi.dv_hys_required, linestyle='--', color='black', label='dv_hys_required')
    plt.plot(hi.time_day, -hi.e_wrap, marker='o', markersize='3', linestyle='None', color='red', label='-e_wrap')
    plt.plot(hi.time_day, hi.dv_hys_redesign_chg, marker=3, markersize='3', linestyle='-', color='green', label='dv_hys_redesign_chg')
    plt.plot(hi.time_day, hi.dv_hys_redesign_dis, marker=3, markersize='3', linestyle='-', color='red', label='dv_hys_redesign_dis')
    plt.xlabel('days')
    plt.legend(loc=4)
    # plt.ylim(-0.7, 0.7)
    plt.ylim(bottom=-0.7)
    plt.subplot(222)
    plt.plot(hi.time_day, hi.res_redesign_chg, marker='o', markersize='3', linestyle='-', color='green', label='res_redesign_chg')
    plt.plot(hi.time_day, hi.res_redesign_dis, marker='o', markersize='3', linestyle='-', color='red', label='res_redesign_dis')
    plt.xlabel('days')
    plt.legend(loc=4)
    plt.subplot(223)
    plt.plot(hi.time_day, hi.ib, color='black', label='ib')
    plt.plot(hi.time_day, hi.soc*10, color='green', label='soc*10')
    plt.plot(hi.time_day, hi.ioc_redesign, marker='o', markersize='3', linestyle='-', color='cyan', label='ioc_redesign')
    plt.xlabel('days')
    plt.legend(loc=4)
    plt.subplot(224)
    plt.plot(hi.time_day, hi.dv_dot_redesign, linestyle='--', color='black', label='dv_dot_redesign')
    plt.xlabel('days')
    plt.legend(loc=4)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 5
    fig_list += 1
    plt.subplot(221)
    plt.title(plot_title + ' h5')
    plt.suptitle(subtitle)
    plt.plot(hi.soc, hi.dv_hys, marker='o', markersize='3', linestyle='-', color='blue', label='dv_hys')
    plt.plot(hi.soc, hi.dv_hys_rescaled, marker='o', markersize='3', linestyle='-', color='cyan', label='dv_hys_rescaled')
    plt.plot(hi.soc, hi.dv_hys_required, linestyle='--', color='black', label='dv_hys_required')
    plt.plot(hi.soc, -hi.e_wrap, marker='o', markersize='3', linestyle='None', color='red', label='-e_wrap')
    plt.plot(hi.soc, hi.dv_hys_redesign_chg, marker=3, markersize='3', linestyle='-', color='green', label='dv_hys_redesign_chg')
    plt.plot(hi.soc, hi.dv_hys_redesign_dis, marker=3, markersize='3', linestyle='-', color='red', label='dv_hys_redesign_dis')
    plt.xlabel('soc')
    plt.legend(loc=4)
    # plt.ylim(-0.7, 0.7)
    plt.ylim(bottom=-0.7)
    plt.subplot(222)
    plt.plot(hi.soc, hi.res_redesign_chg, marker='o', markersize='3', linestyle='-', color='green', label='res_redesign_chg')
    plt.plot(hi.soc, hi.res_redesign_dis, marker='o', markersize='3', linestyle='-', color='red', label='res_redesign_dis')
    plt.xlabel('soc')
    plt.legend(loc=4)
    plt.subplot(223)
    plt.plot(hi.soc, hi.ib, color='black', label='ib')
    plt.plot(hi.soc, hi.soc*10, color='green', label='soc*10')
    plt.plot(hi.soc, hi.ioc_redesign, marker='o', markersize='3', linestyle='-', color='cyan', label='ioc_redesign')
    plt.xlabel('soc')
    plt.legend(loc=4)
    plt.subplot(224)
    plt.plot(hi.soc, hi.dv_dot_redesign, linestyle='--', color='black', label='dv_dot_redesign')
    plt.xlabel('soc')
    plt.legend(loc=4)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return fig_list, fig_files


def over_fault(hi, filename, fig_files=None, plot_title=None, fig_list=None, subtitle=None, long_term=True):
    if fig_files is None:
        fig_files = []

    if long_term:
        plt.figure()  # 1
        fig_list += 1
        plt.subplot(331)
        plt.title(plot_title + ' f1')
        plt.suptitle(subtitle)
        plt.plot(hi.time, hi.soc, marker='.', markersize='3', linestyle='-', color='black', label='soc')
        plt.plot(hi.time, hi.soc_ekf, marker='+', markersize='3', linestyle='--', color='blue', label='soc_ekf')
        plt.legend(loc=1)
        plt.subplot(332)
        plt.plot(hi.time, hi.Tb, marker='.', markersize='3', linestyle='-', color='black', label='Tb')
        plt.legend(loc=1)
        plt.subplot(333)
        plt.plot(hi.time, hi.ib, marker='+', markersize='3', linestyle='-', color='green', label='ib')
        plt.legend(loc=1)
        plt.subplot(334)
        plt.plot(hi.time, hi.tweak_sclr_amp, marker='+', markersize='3', linestyle='None', color='orange', label='tweak_sclr_amp')
        plt.plot(hi.time, hi.tweak_sclr_noa, marker='^', markersize='3', linestyle='None', color='green', label='tweak_sclr_noa')
        plt.ylim(-6, 6)
        plt.legend(loc=1)
        plt.subplot(335)
        plt.plot(hi.time, hi.falw, marker='+', markersize='3', linestyle='None', color='magenta', label='falw')
        plt.legend(loc=0)
        plt.subplot(336)
        plt.plot(hi.soc, hi.soc_ekf, marker='+', markersize='3', linestyle='-', color='blue', label='soc_ekf')
        plt.plot([0, 1], [0.2, 1.2], linestyle=':', color='red')
        plt.plot([0, 1], [-0.2, 0.8], linestyle=':', color='red')
        plt.xlim(0, 1)
        plt.ylim(0, 1)
        plt.xlabel('soc')
        plt.legend(loc=4)
        plt.subplot(337)
        plt.plot(hi.time, hi.dv_hys, marker='o', markersize='3', linestyle='-', color='blue', label='dv_hys')
        plt.plot(hi.time, hi.dv_hys_rescaled, marker='o', markersize='3', linestyle='-', color='cyan', label='dv_hys_rescaled')
        plt.plot(hi.time, hi.dv_hys_required, linestyle='--', color='black', label='dv_hys_required')
        plt.plot(hi.time, -hi.e_wrap, marker='o', markersize='3', linestyle='None', color='red', label='-e_wrap')
        plt.plot(hi.time, hi.dv_hys_remodel, marker='x', markersize='3', linestyle=':', color='lawngreen', label='dv_hys_remodel')
        plt.plot(hi.time, hi.dv_hys_redesign_chg, marker=3, markersize='3', linestyle=':', color='springgreen', label='dv_hys_redesign_chg')
        plt.plot(hi.time, hi.dv_hys_redesign_dis, marker=3, markersize='3', linestyle=':', color='orangered', label='dv_hys_redesign_dis')
        plt.xlabel('days')
        plt.legend(loc=1)
        plt.subplot(338)
        plt.plot(hi.time, hi.e_wrap, marker='o', markersize='3', linestyle='-', color='black', label='e_wrap')
        plt.plot(hi.time, hi.wv_fa, marker=0, markersize='4', linestyle=':', color='red', label='wrap_vb_fa')
        plt.plot(hi.time, hi.wl_fa-1, marker=2, markersize='4', linestyle=':', color='orange', label='wrap_lo_fa-1')
        plt.plot(hi.time, hi.wh_fa+1, marker=3, markersize='4', linestyle=':', color='green', label='wrap_hi_fa+1')
        plt.xlabel('days')
        plt.legend(loc=1)
        plt.subplot(339)
        plt.plot(hi.time, hi.vb, marker='.', markersize='3', linestyle='None', color='red', label='vb')
        plt.plot(hi.time, hi.voc_dyn, marker='.', markersize='3', linestyle='None', color='blue', label='voc_dyn')
        plt.plot(hi.time, hi.voc_stat_chg, marker='.', markersize='3', linestyle='None', color='green', label='voc_stat_chg')
        plt.plot(hi.time, hi.voc_stat_dis, marker='.', markersize='3', linestyle='None', color='red', label='voc_stat_dis')
        plt.xlabel('days')
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # 2
        fig_list += 1
        plt.subplot(221)
        plt.title(plot_title + ' f2')
        plt.suptitle(subtitle)
        plt.plot(hi.time, hi.vsat, marker='.', markersize='1', linestyle='-', color='orange', linewidth='1', label='vsat')
        plt.plot(hi.time, hi.vb, marker='1', markersize='3', linestyle='None', color='black', label='vb')
        plt.plot(hi.time, hi.voc_dyn, marker='.', markersize='3', linestyle='None', color='orange', label='voc_dyn')
        plt.plot(hi.time, hi.voc_stat_chg, marker='.', markersize='3', linestyle='-', color='green', label='voc_stat_chg')
        plt.plot(hi.time, hi.voc_stat_dis, marker='.', markersize='3', linestyle='-', color='red', label='voc_stat_dis')
        plt.plot(hi.time, hi.voc_soc, marker='2', markersize='3', linestyle=':', color='cyan', label='voc_soc')
        plt.xlabel('days')
        plt.legend(loc=1)
        plt.subplot(122)
        plt.plot(hi.time, hi.bms_off + 22, marker='h', markersize='3', linestyle='-', color='blue', label='bms_off+22')
        plt.plot(hi.time, hi.sat + 20, marker='s', markersize='3', linestyle='-', color='red', label='sat+20')
        plt.plot(hi.time, hi.dscn_fa + 18, marker='o', markersize='3', linestyle='-', color='black', label='dscn_fa+18')
        plt.plot(hi.time, hi.ib_diff_fa + 16, marker='^', markersize='3', linestyle='-', color='blue', label='ib_diff_fa+16')
        plt.plot(hi.time, hi.wv_fa + 14, marker='s', markersize='3', linestyle='-', color='cyan', label='wrap_vb_fa+14')
        plt.plot(hi.time, hi.wl_fa + 12, marker='p', markersize='3', linestyle='-', color='orange', label='wrap_lo_fa+12')
        plt.plot(hi.time, hi.wh_fa + 10, marker='h', markersize='3', linestyle='-', color='green', label='wrap_hi_fa+10')
        plt.plot(hi.time, hi.ccd_fa + 8, marker='H', markersize='3', linestyle='-', color='blue', label='cc_diff_fa+8')
        plt.plot(hi.time, hi.ib_noa_fa + 6, marker='+', markersize='3', linestyle='-', color='red', label='ib_noa_fa+6')
        plt.plot(hi.time, hi.ib_amp_fa + 4, marker='_', markersize='3', linestyle='-', color='magenta', label='ib_amp_fa+4')
        plt.plot(hi.time, hi.vb_fa + 2, marker='1', markersize='3', linestyle='-', color='cyan', label='vb_fa+2')
        plt.plot(hi.time, hi.tb_fa, marker='2', markersize='3', linestyle='-', color='orange', label='tb_fa')
        plt.ylim(-1, 24)
        plt.xlabel('days')
        plt.legend(loc=1)
        plt.subplot(223)
        plt.plot(hi.time, hi.ib, marker='.', markersize='3', linestyle='-', color='red', label='ib')
        plt.xlabel('days')
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # 3
        fig_list += 1
        plt.subplot(221)
        plt.title(plot_title + ' f3')
        plt.suptitle(subtitle)
        plt.plot(hi.time, hi.dv_hys, marker='o', markersize='3', linestyle='-', color='blue', label='dv_hys')
        plt.plot(hi.time, hi.dv_hys_rescaled, marker='o', markersize='3', linestyle='-', color='cyan', label='dv_hys_rescaled')
        plt.plot(hi.time, hi.dv_hys_required, linestyle='--', color='black', label='dv_hys_required')
        plt.plot(hi.time, -hi.e_wrap, marker='o', markersize='3', linestyle='None', color='red', label='-e_wrap')
        plt.plot(hi.time, hi.dv_hys_redesign_chg, marker=3, markersize='3', linestyle='-', color='green', label='dv_hys_redesign_chg')
        plt.plot(hi.time, hi.dv_hys_redesign_dis, marker=3, markersize='3', linestyle='-', color='red', label='dv_hys_redesign_dis')
        plt.xlabel('days')
        plt.legend(loc=4)
        # plt.ylim(-0.7, 0.7)
        plt.ylim(bottom=-0.7)
        plt.subplot(222)
        plt.plot(hi.time, hi.res_redesign_chg, marker='o', markersize='3', linestyle='-', color='green', label='res_redesign_chg')
        plt.plot(hi.time, hi.res_redesign_dis, marker='o', markersize='3', linestyle='-', color='red', label='res_redesign_dis')
        plt.xlabel('days')
        plt.legend(loc=4)
        plt.subplot(223)
        plt.plot(hi.time, hi.ib, color='black', label='ib')
        plt.plot(hi.time, hi.soc*10, color='green', label='soc*10')
        plt.plot(hi.time, hi.ioc_redesign, marker='o', markersize='3', linestyle='-', color='cyan', label='ioc_redesign')
        plt.xlabel('days')
        plt.legend(loc=4)
        plt.subplot(224)
        plt.plot(hi.time, hi.dv_dot_redesign, linestyle='--', color='black', label='dv_dot_redesign')
        plt.xlabel('days')
        plt.legend(loc=4)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

    plt.figure()  # 4
    fig_list += 1
    plt.subplot(331)
    plt.title(plot_title + ' f4')
    plt.plot(hi.time, hi.ib, color='green', linestyle='-', label='ib')
    plt.plot(hi.time, hi.ib_diff, color='black', linestyle='-.', label='ib_diff')
    plt.plot(hi.time, hi.ib_diff_thr, color='red', linestyle='-.', label='ib_diff_thr')
    plt.plot(hi.time, -hi.ib_diff_thr, color='red', linestyle='-.')
    plt.legend(loc=1)
    plt.subplot(332)
    plt.plot(hi.time, hi.sat + 2, color='magenta', linestyle='-', label='sat+2')
    plt.legend(loc=1)
    plt.subplot(333)
    plt.plot(hi.time, hi.vb, color='green', linestyle='-', label='vb')
    plt.legend(loc=1)
    plt.subplot(334)
    plt.plot(hi.time, hi.voc_stat, color='green', linestyle='-', label='voc_stat')
    plt.plot(hi.time, hi.vsat, color='blue', linestyle='-', label='vsat')
    plt.plot(hi.time, hi.voc_soc + 0.1, color='black', linestyle='-.', label='voc_soc+0.1')
    plt.plot(hi.time, hi.voc + 0.1, color='green', linestyle=':', label='voc+0.1')
    plt.legend(loc=1)
    plt.subplot(335)
    plt.plot(hi.time, hi.e_w_f, color='black', linestyle='--', label='e_wrap_filt')
    plt.plot(hi.time, hi.ewhi_thr, color='red', linestyle='-.', label='ewhi_thr')
    plt.plot(hi.time, hi.ewlo_thr, color='red', linestyle='-.', label='ewlo_thr')
    plt.ylim(-1, 1)
    plt.legend(loc=1)
    plt.subplot(336)
    plt.plot(hi.time, hi.wh_flt + 4, color='black', linestyle='-', label='wrap_hi_flt+4')
    plt.plot(hi.time, hi.wl_flt + 4, color='magenta', linestyle='--', label='wrap_lo_flt+4')
    plt.plot(hi.time, hi.wh_fa + 2, color='cyan', linestyle='-', label='wrap_hi_fa+2')
    plt.plot(hi.time, hi.wl_fa + 2, color='red', linestyle='--', label='wrap_lo_fa+2')
    plt.plot(hi.time, hi.wv_fa + 2, color='orange', linestyle='-.', label='wrap_vb_fa+2')
    plt.plot(hi.time, hi.ccd_fa, color='green', linestyle='-', label='cc_diff_fa')
    plt.plot(hi.time, hi.red_loss, color='blue', linestyle='--', label='red_loss')
    plt.legend(loc=1)
    plt.subplot(337)
    plt.plot(hi.time, hi.cc_dif, color='black', linestyle='-', label='cc_diff')
    plt.plot(hi.time, hi.cc_diff_thr, color='red', linestyle='--', label='cc_diff_thr')
    plt.plot(hi.time, -hi.cc_diff_thr, color='red', linestyle='--')
    plt.ylim(-.01, .01)
    plt.legend(loc=1)
    plt.subplot(339)
    plt.plot(hi.time, hi.ib_diff_flt + 2, color='cyan', linestyle='-', label='ib_diff_flt+2')
    plt.plot(hi.time, hi.ib_diff_fa + 2, color='magenta', linestyle='--', label='ib_diff_fa+2')
    plt.plot(hi.time, hi.vb_flt, color='blue', linestyle='-.', label='vb_flt')
    plt.plot(hi.time, hi.vb_fa, color='black', linestyle=':', label='vb_fa')
    plt.plot(hi.time, hi.tb_flt, color='red', linestyle='-', label='tb_flt')
    plt.plot(hi.time, hi.tb_fa, color='cyan', linestyle='--', label='tb_fa')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return fig_list, fig_files


def calc_fault(d_ra, d_mod):
    dscn_fa = np.bool_(d_ra.falw & 2 ** 10)
    ib_diff_fa = np.bool_((d_ra.falw & 2 ** 8) | (d_ra.falw & 2 ** 9))
    wv_fa = np.bool_(d_ra.falw & 2 ** 7)
    wl_fa = np.bool_(d_ra.falw & 2 ** 6)
    wh_fa = np.bool_(d_ra.falw & 2 ** 5)
    ccd_fa = np.bool_(d_ra.falw & 2 ** 4)
    ib_noa_fa = np.bool_(d_ra.falw & 2 ** 3)
    ib_amp_fa = np.bool_(d_ra.falw & 2 ** 2)
    vb_fa = np.bool_(d_ra.falw & 2 ** 1)
    tb_fa = np.bool_(d_ra.falw & 2 ** 0)
    e_wrap = d_mod.voc_soc - d_mod.voc
    d_mod = rf.rec_append_fields(d_mod, 'e_wrap', np.array(e_wrap, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'dscn_fa', np.array(dscn_fa, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'ib_diff_fa', np.array(ib_diff_fa, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'wv_fa', np.array(wv_fa, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'wl_fa', np.array(wl_fa, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'wh_fa', np.array(wh_fa, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'ccd_fa', np.array(ccd_fa, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'ib_noa_fa', np.array(ib_noa_fa, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'ib_amp_fa', np.array(ib_amp_fa, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'vb_fa', np.array(vb_fa, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'tb_fa', np.array(tb_fa, dtype=float))

    try:
        ib_diff_flt = np.bool_(d_ra.fltw & 2 ** 8) | (d_ra.fltw & 2 ** 9)
        wh_flt = np.bool_(d_ra.fltw & 2 ** 5)
        wl_flt = np.bool_(d_ra.fltw & 2 ** 6)
        red_loss = np.bool_(d_ra.fltw & 2 ** 7)
        dscn_flt = np.bool_(d_ra.fltw & 2 ** 10)
        vb_flt = np.bool_(d_ra.fltw & 2 ** 1)
        tb_flt = np.bool_(d_ra.fltw & 2 ** 0)
        d_mod = rf.rec_append_fields(d_mod, 'ib_diff_flt', np.array(ib_diff_flt, dtype=float))
        d_mod = rf.rec_append_fields(d_mod, 'wh_flt', np.array(wh_flt, dtype=float))
        d_mod = rf.rec_append_fields(d_mod, 'wl_flt', np.array(wl_flt, dtype=float))
        d_mod = rf.rec_append_fields(d_mod, 'red_loss', np.array(red_loss, dtype=float))
        d_mod = rf.rec_append_fields(d_mod, 'dscn_flt', np.array(dscn_flt, dtype=float))
        d_mod = rf.rec_append_fields(d_mod, 'vb_flt', np.array(vb_flt, dtype=float))
        d_mod = rf.rec_append_fields(d_mod, 'tb_flt', np.array(tb_flt, dtype=float))
    except:
        pass

    return d_mod


# Fake stuff to get replicate to accept inputs and run
def bandaid(h):
    res = np.zeros(len(h.time))
    res[0:10] = 1
    mod = np.zeros(len(h.time))
    ib_in_s = h['ib'].copy()
    soc_s = h['soc'].copy()
    sat_s = h['sat'].copy()
    chm = np.zeros(len(h.time))
    chm_s = np.zeros(len(h.time))
    mon_old = rf.rec_append_fields(h, 'res', res)
    mon_old = rf.rec_append_fields(mon_old, 'mod_data', mod)
    mon_old = rf.rec_append_fields(mon_old, 'ib_past', ib_in_s)
    mon_old = rf.rec_append_fields(mon_old, 'soc_s', soc_s)
    mon_old = rf.rec_append_fields(mon_old, 'chm', chm)
    sim_old = np.array(np.zeros(len(h.time), dtype=[('time', '<i4')])).view(np.recarray)
    sim_old.time = mon_old.time.copy()
    sim_old = rf.rec_append_fields(sim_old, 'chm_s', chm_s)
    sim_old = rf.rec_append_fields(sim_old, 'sat_s', sat_s)
    sim_old = rf.rec_append_fields(sim_old, 'ib_in_s', ib_in_s)
    return mon_old, sim_old


def calculate_capacity(q_cap_rated_scaled=None, dqdt=None, temp=None, t_rated=None):
    q_cap = q_cap_rated_scaled * (1. + dqdt * (temp - t_rated))
    return q_cap


# Table lookup vector
def look_it(x, tab, temp):
    voc = x.copy()
    for i in range(len(x)):
        voc[i] = tab.interp(x[i], temp)
    return voc


if __name__ == '__main__':
    import sys
    from DataOverModel import write_clean_file
    from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
    plt.rcParams['axes.grid'] = True
    from datetime import datetime


    def main():
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        # date_ = datetime.now().strftime("%y%m%d")
        skip = 1

        # Battleborn Bmon=0, Bsim=0
        t_x_soc0 = [-0.15, 0.00, 0.05, 0.10, 0.14, 0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99,  0.995, 1.00]
        t_y_t0 = [5.,  11.1,  20.,   30.,   40.]
        t_voc0 = [4.00, 4.00,  9.00,  11.80, 12.50, 12.60, 12.67, 12.76, 12.82, 12.93, 12.98, 13.03, 13.07, 13.11, 13.17, 13.22, 13.59, 14.45,
                  4.00, 4.00,  10.00, 12.30, 12.60, 12.65, 12.71, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 13.60, 14.46,
                  4.00, 12.22, 12.66, 12.74, 12.80, 12.85, 12.89, 12.95, 12.99, 13.05, 13.08, 13.13, 13.18, 13.21, 13.25, 13.27, 13.72, 14.50,
                  4.00, 4.00, 12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26, 13.27, 13.72, 14.50,
                  4.00, 4.00, 4.00,  4.00,  10.50, 11.93, 12.78, 12.83, 12.89, 12.97, 13.06, 13.10, 13.13, 13.16, 13.19, 13.20, 13.72, 14.50]
        x0 = np.array(t_x_soc0)
        y0 = np.array(t_y_t0)
        data_interp0 = np.array(t_voc0)
        lut_voc = myTables.TableInterp2D(x0, y0, data_interp0)
        t_x_soc_min = [5., 11.1,  20.,   30., 40.]
        t_soc_min = [0.07, 0.05,  -0.05, 0.00, 0.20]
        lut_soc_min = myTables.TableInterp1D(np.array(t_x_soc_min), np.array(t_soc_min))

        # Save these
        t_max_in = None
        rated_batt_cap_in = 108.4

        # User inputs
        # input_files = ['coldCharge1 v20221028.txt']
        # input_files = ['fault_20221206.txt']
        # input_files = ['hist_v20221028c_20221213.txt']
        # input_files = ['g20230530/Hd_20230714_soc1a_bb.csv']; chm_in = 0; rated_batt_cap_in = 108.4;
        input_files = ['g20230530/serial_20231002_104351.csv']; chm_in = 0; rated_batt_cap_in = 108.4
        # input_files = ['CH 20230128.txt']
        # t_max_in = 20.
        # exclusions = [(0, 1665334404)]  # before faults
        # exclusions = [(0, 1665404608), (1665433410, 1670000000)]  # EKF wander full
        # exclusions = [(0, 1665404608), (1665419009, 1670000000)]  # EKF wander first part
        exclusions = []

        temp_hist_file = 'hist20221028.txt'
        temp_flt_file = 'flt20221028.txt'
        path_to_pdfs = '../dataReduction/figures'
        path_to_data = '../dataReduction'
        path_to_temp = '../dataReduction/temp'
        import os
        if not os.path.isdir(path_to_temp):
            os.mkdir(path_to_temp)
        cols_f = ('time', 'Tb_h', 'vb_h', 'ibmh', 'ibnh', 'Tb', 'vb', 'ib', 'soc', 'soc_ekf', 'voc', 'voc_stat',
                  'e_w_f', 'fltw', 'falw')
        # cat files
        cat(temp_hist_file, input_files, in_path=path_to_data, out_path=path_to_temp)

        # Load history
        temp_hist_file = os.path.join(path_to_temp, temp_hist_file)
        temp_hist_file_clean = write_clean_file(temp_hist_file, type_='', title_key='fltb', unit_key='unit_f',
                                                skip=skip, comment_str='---')
        if temp_hist_file_clean:
            h_raw = np.genfromtxt(temp_hist_file_clean, delimiter=',', names=True, usecols=cols_f, dtype=None,
                                  encoding=None).view(np.recarray)
        else:
            print("data from temp hist", temp_hist_file, "empty after loading")
            return None, None, None, None, None

        # Load fault
        temp_flt_file_clean = write_clean_file(temp_hist_file, type_='', title_key='fltb', unit_key='unit_f',
                                               skip=skip, comment_str='---')
        if temp_flt_file_clean:
            f_raw = np.genfromtxt(temp_flt_file_clean, delimiter=',', names=True, usecols=cols_f, dtype=None,
                                  encoding=None).view(np.recarray)
        else:
            print("data from temp flt", temp_flt_file, "empty after loading")
            return None, None, None, None, None

        # Load configuration
        from Battery import BatteryMonitor
        batt = BatteryMonitor(mod_code=chm_in)

        f_raw = np.unique(f_raw)
        f = add_stuff_f(f_raw, batt, ib_band=IB_BAND, rated_batt_cap=rated_batt_cap_in)
        print("\nf:\n", f, "\n")
        f = filter_Tb(f, 20., batt, tb_band=100., rated_batt_cap=UNIT_CAP_RATED)

        # Sort unique
        h_raw = np.unique(h_raw)
        # Rack and stack
        if exclusions:
            for i in range(len(exclusions)):
                test_res0 = np.where(h_raw.time < exclusions[i][0])
                test_res1 = np.where(h_raw.time > exclusions[i][1])
                h_raw = h_raw[np.hstack((test_res0, test_res1))[0]]
        h = add_stuff(h_raw, voc_soc_tbl=lut_voc, soc_min_tbl=lut_soc_min, ib_band=IB_BAND)
        print("\nh:\n", h, "\n")
        # voc_soc05 = look_it(x0, lut_voc, 5.)
        # h_05C = filter_Tb(h, 5., tb_band=TB_BAND, rated_batt_cap=UNIT_CAP_RATED)
        # voc_soc11 = look_it(x0, lut_voc, 11.1)
        # h_11C = filter_Tb(h, 11.1, tb_band=TB_BAND, rated_batt_cap=UNIT_CAP_RATED)
        # voc_soc20 = look_it(x0, lut_voc, 20.)
        # h_20C = filter_Tb(h, 20., tb_band=TB_BAND, rated_batt_cap=UNIT_CAP_RATED)
        # voc_soc30 = look_it(x0, lut_voc, 30.)
        # h_30C = filter_Tb(h, 30., tb_band=TB_BAND, rated_batt_cap=UNIT_CAP_RATED)
        # voc_soc40 = look_it(x0, lut_voc, 40.)
        # h_40C = filter_Tb(h, 40., tb_band=TB_BAND, rated_batt_cap=UNIT_CAP_RATED)
        voc_soc20 = look_it(x0, lut_voc, 20.)
        print('filter h')
        h_20C = filter_Tb(h, 20., tb_band=TB_BAND, rated_batt_cap=UNIT_CAP_RATED)
        T_300new = 0.3  # still allows Randles to run (t_max=0.31 in Battery.py)
        T_300old = 0.3  # still allows Randles to run (t_max=0.31 in Battery.py)
        # T_300new = 10  # For long histories
        # T_300old = 10  # For long histories
        print('resample old')
        h_20C_resamp_300old = resample(data=h_20C, dt_resamp=T_300old, time_var='time',
                                       specials=[('falw', 0), ('dscn_fa', 0), ('ib_diff_fa', 0), ('wv_fa', 0),
                                                 ('wl_fa', 0), ('wh_fa', 0), ('ccd_fa', 0), ('ib_noa_fa', 0),
                                                 ('ib_amp_fa', 0), ('vb_fa', 0), ('tb_fa', 0)])
        print('bandaid old')
        mon_old_300old, sim_old_300old = bandaid(h_20C_resamp_300old)
        print('replicate old')
        mon_ver_300old, sim_ver_300old, randles_ver_300old, sim_s_ver_300old =\
            replicate(mon_old_300old, sim_old=sim_old_300old, init_time=1., verbose=False, t_max=t_max_in,
                      eframe_mult=1)
        print('resample new')
        h_20C_resamp_300new = resample(data=h_20C, dt_resamp=T_300new, time_var='time',
                                       specials=[('falw', 0), ('dscn_fa', 0), ('ib_diff_fa', 0), ('wv_fa', 0),
                                                 ('wl_fa', 0), ('wh_fa', 0), ('ccd_fa', 0), ('ib_noa_fa', 0),
                                                 ('ib_amp_fa', 0), ('vb_fa', 0), ('tb_fa', 0)])
        print('bandaid new')
        mon_old_300new, sim_old_300new = bandaid(h_20C_resamp_300new)
        eframe_mult = max(int(0.1*cp_eframe_mult / T_300new), 1)
        print("replicate new")
        mon_ver_300new, sim_ver_300new, randles_ver_300new, sim_s_ver_300new =\
            replicate(mon_old_300new, sim_old=sim_old_300new, init_time=max(1., T_300new), verbose=False, t_max=t_max_in,
                      eframe_mult=eframe_mult)

        # Plots
        fig_list = []
        fig_files = []
        data_root = temp_hist_file_clean.split('/')[-1].replace('.csv', '-')
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        # if len(h_05C.time) > 1:
        #     fig_list, fig_files = over_easy(h_05C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_05C', fig_list=fig_list, x_sch=x0, z_sch=voc_soc05, voc_reset=VOC_RESET_05)
        # if len(h_11C.time) > 1:
        #     fig_list, fig_files = over_easy(h_11C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_11C', fig_list=fig_list, x_sch=x0, z_sch=voc_soc11, voc_reset=VOC_RESET_11)
        # if len(h_20C.time) > 1:
        #     fig_list, fig_files = over_easy(h_20C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_20C', fig_list=fig_list, x_sch=x0, z_sch=voc_soc20, voc_reset=VOC_RESET_20)
        # if len(h_30C.time) > 1:
        #     fig_list, fig_files = over_easy(h_30C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_30C', fig_list=fig_list, x_sch=x0, z_sch=voc_soc30, voc_reset=VOC_RESET_30)
        # if len(h_40C.time) > 1:
        #     fig_list, fig_files = over_easy(h_40C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_40C', fig_list=fig_list, x_sch=x0, z_sch=voc_soc40, voc_reset=VOC_RESET_40)
        if len(h_20C.time) > 1:
            print("over_easy")
            fig_list, fig_files = over_easy(h_20C, filename, mv_fast=mon_ver_300new, mv_slow=mon_ver_300old,
                                         fig_files=fig_files, plot_title=plot_title, subtitle='h_20C',
                                         fig_list=fig_list, x_sch=x0, z_sch=voc_soc20, voc_reset=VOC_RESET_20)
            print("overall_batt")
            fig_list, fig_files = overall_batt(mon_ver_300old, sim_ver_300old, randles_ver_300old, suffix='_300old',
                                            filename=filename, fig_files=fig_files,
                                            mv1=mon_ver_300new, sv1=sim_ver_300new, rv1=randles_ver_300new,
                                            suffix1='_300new', plot_title=plot_title, fig_list=fig_list, use_time_day=True)
        if len(f.time) > 1:
            print('over_fault')
            fig_list, fig_files = over_fault(f, filename, fig_files=fig_files, plot_title=plot_title, subtitle='faults',
                                          fig_list=fig_list, x_sch=x0, z_sch=voc_soc20, voc_reset=VOC_RESET_20)

        precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=path_to_pdfs)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', save_pdf_path=path_to_pdfs)
        cleanup_fig_files(fig_files)

        plt.show()

    main()