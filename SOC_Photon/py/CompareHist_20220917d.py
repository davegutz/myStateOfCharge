# MonSim:  Monitor and Simulator replication of Particle Photon Application
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

""" Slice and dice the history dumps."""

from pyDAGx import myTables
import numpy as np
import numpy.lib.recfunctions as rf
import matplotlib.pyplot as plt
from Hysteresis_20220917d import Hysteresis_20220917d
from Hysteresis_20220926 import Hysteresis_20220926

#  For this battery Battleborn 100 Ah with 1.084 x capacity
BATT_RATED_TEMP = 25.  # Temperature at RATED_BATT_CAP, deg C
BATT_V_SAT = 13.8
BATT_DQDT = 0.01  # Change of charge with temperature, fraction / deg C (0.01 from literature)
BATT_DVOC_DT = 0.004  # Change of VOC with operating temperature in range 0 - 50 C V / deg C
RATED_BATT_CAP = 108.4  # A-hr capacity of test article
IB_BAND = 1.  # Threshold to declare charging or discharging
TB_BAND = 5.  # Band around temperature to group data and correct
HYS_SCALE_20220917d = 0.3  # Original hys_remodel scalar inside photon code
HYS_SCALE_20220926 = 1.0  # Original hys_remodel scalar inside photon code

#  Rescale parameters design.  Minimal tuning attempt
#  This didn't work because low soc response of original design is too slow
HYS_RESCALE_CHG = 0.5  # Attempt to rescale to match voc_soc to all data
HYS_RESCALE_DIS = 0.3  # Attempt to rescale to match voc_soc to all data
VOC_RESET_05 = 0.  # Attempt to rescale to match voc_soc to all data
VOC_RESET_11 = 0.  # Attempt to rescale to match voc_soc to all data
VOC_RESET_20 = 0.  # Attempt to rescale to match voc_soc to all data
VOC_RESET_30 = -0.03  # Attempt to rescale to match voc_soc to all data
VOC_RESET_40 = 0.  # Attempt to rescale to match voc_soc to all data

#  Redesign Hysteresis_20220917d.  Make a new Hysteresis_20220926.py with new curve
HYS_CAP_REDESIGN = 3.6e4  # faster time constant needed
HYS_RESHAPE_CHG = 1.  # start again
HYS_RESHAPE_DIS = 1.  # start again

# Unix-like cat function
# e.g. > cat('out', ['in0', 'in1'], path_to_in='./')
def cat(out_file_name, in_file_names, in_path='./', out_path='./'):
    with open(out_path+'./'+out_file_name, 'w') as out_file:
        for in_file_name in in_file_names:
            with open(in_path+'/'+in_file_name) as in_file:
                for line in in_file:
                    if line.strip():
                        out_file.write(line)


def over_easy(hi, filename, fig_files=None, plot_title=None, n_fig=None, subtitle=None,  x_sch=None, z_sch=None,
              voc_reset=0.):
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
    n_fig += 1
    plt.subplot(331)
    plt.title(plot_title)
    plt.suptitle(subtitle)
    plt.plot(hi.time_day, hi.soc, marker='.', markersize='3', linestyle='-', color='black', label='soc')
    plt.plot(hi.time_day, hi.soc_ekf, marker='+', markersize='3', linestyle='--', color='blue', label='soc_ekf')
    plt.legend(loc=1)
    plt.subplot(332)
    plt.plot(hi.time_day, hi.Tb, marker='.', markersize='3', linestyle='-', color='black', label='Tb')
    plt.legend(loc=1)
    plt.subplot(333)
    plt.plot(hi.time_day, hi.Ib, marker='+', markersize='3', linestyle='-', color='green', label='Ib')
    plt.legend(loc=1)
    plt.subplot(334)
    plt.plot(hi.time_day, hi.tweak_sclr_amp, marker='+', markersize='3', linestyle='None', color='orange', label='tweak_sclr_amp')
    plt.plot(hi.time_day, hi.tweak_sclr_noa, marker='^', markersize='3', linestyle='None', color='green', label='tweak_sclr_noa')
    plt.ylim(-6, 6)
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
    plt.plot(hi.time_day, hi.dv_hys_redesign, marker=3, markersize='3', linestyle=':', color='pink', label='dv_hys_redesign')
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
    plt.plot(hi.time_day, hi.Vb, marker='.', markersize='3', linestyle='None', color='red', label='Vb')
    plt.plot(hi.time_day, hi.Voc_dyn, marker='.', markersize='3', linestyle='None', color='blue', label='Voc_dyn')
    plt.plot(hi.time_day, hi.Voc_stat_chg, marker='.', markersize='3', linestyle='None', color='red', label='Voc_stat_chg')
    plt.plot(hi.time_day, hi.Voc_stat_dis, marker='.', markersize='3', linestyle='None', color='green', label='Voc_stat_dis')
    plt.xlabel('days')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 2
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title)
    plt.suptitle(subtitle)
    plt.plot(hi.time_day, hi.Vsat, marker='.', markersize='1', linestyle='-', color='orange', linewidth='1', label='Vsat')
    plt.plot(hi.time_day, hi.Vb, marker='1', markersize='3', linestyle='None', color='black', label='Vb')
    plt.plot(hi.time_day, hi.Voc_dyn, marker='.', markersize='3', linestyle='None', color='orange', label='Voc_dyn')
    plt.plot(hi.time_day, hi.Voc_stat_chg, marker='.', markersize='3', linestyle='-', color='red', label='Voc_stat_chg')
    plt.plot(hi.time_day, hi.Voc_stat_dis, marker='.', markersize='3', linestyle='-', color='green', label='Voc_stat_dis')
    plt.plot(hi.time_day, hi.voc_soc, marker='2', markersize='3', linestyle=':', color='cyan', label='voc_soc')
    plt.xlabel('days')
    plt.legend(loc=1)
    plt.subplot(122)
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
    plt.plot(hi.time_day, hi.Ib, marker='.', markersize='3', linestyle='-', color='red', label='Ib')
    plt.xlabel('days')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 3
    n_fig += 1
    plt.subplot(131)
    plt.title(plot_title)
    plt.suptitle(subtitle)
    plt.plot(hi.soc_r, hi.Voc_stat_r_dis, marker='o', markersize='3', linestyle='-', color='cyan', label='Voc_stat_r_dis')
    plt.plot(hi.soc_r, hi.Voc_stat_r_chg, marker='o', markersize='3', linestyle='-', color='orange', label='Voc_stat_r_chg')
    plt.plot(hi.soc, hi.Voc_stat_dis, marker='.', markersize='3', linestyle='None', color='green', label='Voc_stat_dis')
    plt.plot(hi.soc, hi.Voc_stat_chg, marker='.', markersize='3', linestyle='None', color='red', label='Voc_stat_chg')
    plt.plot(x_sch, z_sch, marker='+', markersize='2', linestyle='--', color='black', label='Schedule')
    plt.xlabel('soc')
    plt.legend(loc=4)
    plt.ylim(12, 13.5)
    plt.subplot(132)
    plt.plot(hi.soc_r, hi.Voc_stat_rescaled_r_dis, marker='o', markersize='3', linestyle='-', color='cyan', label='Voc_stat_rescaled_r_dis')
    plt.plot(hi.soc_r, hi.Voc_stat_rescaled_r_chg, marker='o', markersize='3', linestyle='-', color='orange', label='Voc_stat_rescaled_r_chg')
    plt.plot(x_sch, z_sch+voc_reset, marker='+', markersize='2', linestyle='--', color='black', label='Schedule RESET')
    plt.xlabel('soc_r')
    plt.legend(loc=4)
    plt.ylim(12, 13.5)
    plt.subplot(133)
    plt.plot(hi.soc_r, hi.Voc_stat_redesign_r_dis, marker='x', markersize='3', linestyle='-', color='magenta', label='Voc_stat_redesign_r_dis')
    plt.plot(hi.soc_r, hi.Voc_stat_redesign_r_chg, marker='x', markersize='3', linestyle='-', color='springgreen', label='Voc_stat_redesign_r_chg')
    plt.plot(x_sch, z_sch+voc_reset, marker='+', markersize='2', linestyle='--', color='black', label='Schedule RESET')
    plt.xlabel('soc_r')
    plt.legend(loc=4)
    plt.ylim(12, 13.5)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 4
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title)
    plt.suptitle(subtitle)
    plt.plot(hi.time_day, hi.dv_hys, marker='o', markersize='3', linestyle='-', color='blue', label='dv_hys')
    plt.plot(hi.time_day, hi.dv_hys_rescaled, marker='o', markersize='3', linestyle='-', color='cyan', label='dv_hys_rescaled')
    plt.plot(hi.time_day, hi.dv_hys_required, linestyle='--', color='black', label='dv_hys_required')
    plt.plot(hi.time_day, -hi.e_wrap, marker='o', markersize='3', linestyle='None', color='red', label='-e_wrap')
    plt.plot(hi.time_day, hi.dv_hys_remodel, marker='x', markersize='3', linestyle=':', color='lawngreen', label='dv_hys_remodel')
    plt.plot(hi.time_day, hi.dv_hys_redesign, marker=3, markersize='3', linestyle=':', color='pink', label='dv_hys_redesign')
    plt.xlabel('days')
    plt.legend(loc=4)
    plt.ylim(-0.7, 0.7)
    plt.subplot(222)
    plt.plot(hi.time_day, hi.res_redesign, marker='o', markersize='3', linestyle='-', color='blue', label='res_redesign')
    plt.xlabel('days')
    plt.legend(loc=4)
    plt.subplot(223)
    plt.plot(hi.time_day, hi.Ib, color='black', label='Ib')
    plt.plot(hi.time_day, hi.soc*10, color='green', label='soc*10')
    plt.plot(hi.time_day, hi.ioc_redesign, marker='o', markersize='3', linestyle='-', color='cyan', label='ioc_redesign')
    plt.xlabel('days')
    plt.legend(loc=4)
    plt.subplot(224)
    plt.plot(hi.time_day, hi.dv_dot_redesign, linestyle='--', color='black', label='dv_dot_redesign')
    plt.xlabel('days')
    plt.legend(loc=4)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 5
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title)
    plt.suptitle(subtitle)
    plt.plot(hi.soc, hi.dv_hys, marker='o', markersize='3', linestyle='-', color='blue', label='dv_hys')
    plt.plot(hi.soc, hi.dv_hys_rescaled, marker='o', markersize='3', linestyle='-', color='cyan', label='dv_hys_rescaled')
    plt.plot(hi.soc, hi.dv_hys_required, linestyle='--', color='black', label='dv_hys_required')
    plt.plot(hi.soc, -hi.e_wrap, marker='o', markersize='3', linestyle='None', color='red', label='-e_wrap')
    plt.plot(hi.soc, hi.dv_hys_remodel, marker='x', markersize='3', linestyle=':', color='lawngreen', label='dv_hys_remodel')
    plt.plot(hi.soc, hi.dv_hys_redesign, marker=3, markersize='3', linestyle=':', color='pink', label='dv_hys_redesign')
    plt.xlabel('soc')
    plt.legend(loc=4)
    plt.ylim(-0.7, 0.7)
    plt.subplot(222)
    plt.plot(hi.soc, hi.res_redesign, marker='o', markersize='3', linestyle='-', color='blue', label='res_redesign')
    plt.xlabel('soc')
    plt.legend(loc=4)
    plt.subplot(223)
    plt.plot(hi.soc, hi.Ib, color='black', label='Ib')
    plt.plot(hi.soc, hi.soc*10, color='green', label='soc*10')
    plt.plot(hi.soc, hi.ioc_redesign, marker='o', markersize='3', linestyle='-', color='cyan', label='ioc_redesign')
    plt.xlabel('soc')
    plt.legend(loc=4)
    plt.subplot(224)
    plt.plot(hi.soc, hi.dv_dot_redesign, linestyle='--', color='black', label='dv_dot_redesign')
    plt.xlabel('soc')
    plt.legend(loc=4)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files


# Add schedule lookups and do some rack and stack
def add_stuff(d_ra, voc_soc_tbl, ib_band=0.5):
    voc_soc = []
    Vsat = []
    time_sec = []
    for i in range(len(d_ra.time)):
        voc_soc.append(voc_soc_tbl.interp(d_ra.soc[i], d_ra.Tb[i]))
        Vsat.append(BATT_V_SAT + (d_ra.Tb[i] - BATT_RATED_TEMP) * BATT_DVOC_DT)
        time_sec.append(float(d_ra.time[i] - d_ra.time[0]))
    d_mod = rf.rec_append_fields(d_ra, 'voc_soc', np.array(voc_soc, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'Vsat', np.array(Vsat, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'time_sec', np.array(time_sec, dtype=float))
    dscn_fa = np.bool8(d_ra.falw & 2 ** 10)
    ib_diff_fa = np.bool8((d_ra.falw & 2 ** 8) | (d_ra.falw & 2 ** 9))
    wv_fa = np.bool8(d_ra.falw & 2 ** 7)
    wl_fa = np.bool8(d_ra.falw & 2 ** 6)
    wh_fa = np.bool8(d_ra.falw & 2 ** 5)
    ccd_fa = np.bool8(d_ra.falw & 2 ** 4)
    ib_noa_fa = np.bool8(d_ra.falw & 2 ** 3)
    ib_amp_fa = np.bool8(d_ra.falw & 2 ** 2)
    vb_fa = np.bool8(d_ra.falw & 2 ** 1)
    tb_fa = np.bool8(d_ra.falw & 2 ** 0)
    e_wrap = d_mod.voc_soc - d_mod.Voc_dyn
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
    Voc_stat_chg = np.copy(d_mod.Voc_stat)
    Voc_stat_dis = np.copy(d_mod.Voc_stat)
    for i in range(len(Voc_stat_chg)):
        if d_mod.Ib[i] > -ib_band:
            Voc_stat_dis[i] = None
        elif d_mod.Ib[i] < ib_band:
            Voc_stat_chg[i] = None
    d_mod = rf.rec_append_fields(d_mod, 'Voc_stat_chg', np.array(Voc_stat_chg, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'Voc_stat_dis', np.array(Voc_stat_dis, dtype=float))
    time_day = (d_mod.time-d_mod.time[0])/3600./24.
    d_mod = rf.rec_append_fields(d_mod, 'time_day', np.array(time_day, dtype=float))
    time_min = (d_mod.time-d_mod.time[0])/60.
    d_mod = rf.rec_append_fields(d_mod, 'time_min', np.array(time_min, dtype=float))
    dv_hys = d_mod.Voc_dyn - d_mod.Voc_stat
    d_mod = rf.rec_append_fields(d_mod, 'dv_hys', np.array(dv_hys, dtype=float))
    dv_hys_unscaled = d_mod.dv_hys / HYS_SCALE_20220917d
    d_mod = rf.rec_append_fields(d_mod, 'dv_hys_unscaled', np.array(dv_hys_unscaled, dtype=float))
    dv_hys_required = d_mod.Voc_dyn - voc_soc
    d_mod = rf.rec_append_fields(d_mod, 'dv_hys_required', np.array(dv_hys_required, dtype=float))

    dv_hys_rescaled = d_mod.dv_hys_unscaled
    pos = dv_hys_rescaled >= 0
    neg = dv_hys_rescaled < 0
    dv_hys_rescaled[pos] *= HYS_RESCALE_CHG
    dv_hys_rescaled[neg] *= HYS_RESCALE_DIS
    d_mod = rf.rec_append_fields(d_mod, 'dv_hys_rescaled', np.array(dv_hys_rescaled, dtype=float))
    Voc_stat_rescaled = d_mod.Voc_dyn - d_mod.dv_hys_rescaled
    d_mod = rf.rec_append_fields(d_mod, 'Voc_stat_rescaled', np.array(Voc_stat_rescaled, dtype=float))

    return d_mod


def calculate_capacity(q_cap_rated_scaled=None, dqdt=None, temp=None, t_rated=None):
    q_cap = q_cap_rated_scaled * (1. + dqdt * (temp - t_rated))
    return q_cap


# Make an array useful for analysis (around temp) and add some metrics
def filter_Tb(raw, temp_corr, tb_band=5., rated_batt_cap=100.):
    h = raw[abs(raw.Tb - temp_corr) < tb_band]

    # Correct for temp
    q_cap = calculate_capacity(q_cap_rated_scaled=rated_batt_cap * 3600., dqdt=BATT_DQDT, temp=h.Tb, t_rated=BATT_RATED_TEMP)
    dq = (h.soc - 1.) * q_cap
    dq -= BATT_DQDT * q_cap * (temp_corr - h.Tb)
    q_cap_r = calculate_capacity(q_cap_rated_scaled=rated_batt_cap * 3600., dqdt=BATT_DQDT, temp=temp_corr, t_rated=BATT_RATED_TEMP)
    h.soc_r = 1. + dq / q_cap_r
    h.Voc_stat_r = h.Voc_stat - (h.Tb - temp_corr) * BATT_DVOC_DT
    h.Voc_stat_rescaled_r = h.Voc_stat_rescaled - (h.Tb - temp_corr) * BATT_DVOC_DT

    # delineate charging and discharging
    h.Voc_stat_r_chg = np.copy(h.Voc_stat)
    h.Voc_stat_r_dis = np.copy(h.Voc_stat)
    h.Voc_stat_rescaled_r_chg = np.copy(h.Voc_stat_rescaled)
    h.Voc_stat_rescaled_r_dis = np.copy(h.Voc_stat_rescaled)
    for i in range(len(h.Voc_stat_r_chg)):
        if h.Ib[i] > -0.5:
            h.Voc_stat_r_dis[i] = None
            h.Voc_stat_rescaled_r_dis[i] = None
        elif h.Ib[i] < 0.5:
            h.Voc_stat_r_chg[i] = None
            h.Voc_stat_rescaled_r_chg[i] = None

    # Hysteresis_20220917d confirm equals data with HYS_SCALE_20220917d
    if len(h.time) > 1:
        hys_remodel = Hysteresis_20220917d(scale=HYS_SCALE_20220917d)  # Battery hysteresis model - drift of voc
        t_s_min = h.time_min[0]
        t_e_min = h.time_min[-1]
        dt_hys_min = 1.
        dt_hys_sec = dt_hys_min * 60.
        hys_time_min = np.arange(t_s_min, t_e_min, dt_hys_min, dtype=float)
        ib_vec = h.Ib
        # Note:  Hysteresis_20220917d instantiates hysteresis state to 0. unless told otherwise
        dv_hys_remodel = []
        h_time_sec = h.time
        for i in range(len(hys_time_min)):
            t_sec = hys_time_min[i] * 60.
            ib = np.interp(t_sec, h.time_sec, h.Ib)
            soc = np.interp(t_sec, h.time_sec, h.soc)
            hys_remodel.calculate_hys(ib, soc)
            dvh = hys_remodel.update(dt_hys_sec)
            dv_hys_remodel.append(dvh)
        dv_hys_remodel = np.array(dv_hys_remodel)
        h.dv_hys_remodel = np.copy(h.soc)
        for i in range(len(h.time)):
            t_min = int(float(h.time[i]) / 60.)
            h.dv_hys_remodel[i] = np.interp(t_min, hys_time_min, dv_hys_remodel)

    # Hysteresis_20220926 redesign.
    if len(h.time) > 1:
        hys_redesign = Hysteresis_20220926(scale=HYS_SCALE_20220926, cap=HYS_CAP_REDESIGN)  # Battery hysteresis model - drift of voc
        t_s_min = h.time_min[0]
        t_e_min = h.time_min[-1]
        dt_hys_min = 1.
        dt_hys_sec = dt_hys_min * 60.
        hys_time_min = np.arange(t_s_min, t_e_min, dt_hys_min, dtype=float)
        ib_vec = h.Ib
        # Note:  Hysteresis_20220926 instantiates hysteresis state to 0. unless told otherwise
        dv_hys_redesign = []
        res_redesign = []
        ioc_redesign = []
        dv_dot_redesign = []
        voc_stat_redesign = []
        voc_stat_redesign_r = []
        print("top redesign loop:  t_s_min", t_s_min, "t_e_min", t_e_min, "hys_t_m", hys_time_min)
        for i in range(len(hys_time_min)):
            t_sec = hys_time_min[i] * 60
            tb = np.interp(t_sec, h.time_sec, h.Tb)
            ib = np.interp(t_sec, h.time_sec, h.Ib)
            soc = np.interp(t_sec, h.time_sec, h.soc)
            Voc = np.interp(t_sec, h.time_sec, h.Voc_dyn)
            hys_redesign.calculate_hys(ib, soc)
            dvh = hys_redesign.update(dt_hys_sec)
            if soc<.1:
                print("creation loop: t_s_min, t_e_min, min, days, soc, A, dV", t_s_min, t_e_min, hys_time_min[i], hys_time_min[i]/60./24., soc, ib, dvh)
            res = hys_redesign.res
            ioc = hys_redesign.ioc
            dv_dot = hys_redesign.dv_dot
            voc_stat = Voc - dvh
            voc_stat_r = voc_stat - (tb - temp_corr) * BATT_DVOC_DT
            dv_hys_redesign.append(dvh)
            res_redesign.append(res)
            ioc_redesign.append(ioc)
            dv_dot_redesign.append(dv_dot)
            voc_stat_redesign.append(voc_stat)
            voc_stat_redesign_r.append(voc_stat_r)
        dv_hys_redesign = np.array(dv_hys_redesign)
        res_redesign = np.array(res_redesign)
        ioc_redesign = np.array(ioc_redesign)
        dv_dot_redesign = np.array(dv_dot_redesign)
        h.dv_hys_redesign = np.copy(h.soc)
        h.res_redesign = np.copy(h.soc)
        h.ioc_redesign = np.copy(h.soc)
        h.dv_dot_redesign = np.copy(h.soc)
        h.Voc_stat_redesign = np.copy(h.soc)
        h.Voc_stat_redesign_r = np.copy(h.soc)
        dtsec = h.time[-1]-h.time[0]
        dtmin = dtsec/60.
        dthr = dtmin/60.
        dtday = dthr/24.
        print("h.time: beg, end, delta", h.time[0], h.time[-1], dtday)
        for i in range(len(h.time)):
            t_min = int(float(h.time[i] - h.time[0]) / 60.)
            t_hr = t_min / 60.
            t_days = t_hr / 24.
            h.dv_hys_redesign[i] = np.interp(t_min, hys_time_min, dv_hys_redesign)
            if h.soc[i] < .1:
                print("interp: min, days, dV", t_min, t_days, h.soc[i], h.Ib[i], h.dv_hys_redesign[i])
            h.res_redesign[i] = np.interp(t_min, hys_time_min, res_redesign)
            h.ioc_redesign[i] = np.interp(t_min, hys_time_min, ioc_redesign)
            h.dv_dot_redesign[i] = np.interp(t_min, hys_time_min, dv_dot_redesign)
            h.Voc_stat_redesign[i] = np.interp(t_min, hys_time_min, voc_stat_redesign)
            h.Voc_stat_redesign_r[i] = np.interp(t_min, hys_time_min, voc_stat_redesign_r)
        h.Voc_stat_redesign_r_chg = np.copy(h.Voc_stat_redesign_r)
        h.Voc_stat_redesign_r_dis = np.copy(h.Voc_stat_redesign_r)
        for i in range(len(h.Voc_stat_r_chg)):
            if h.Ib[i] > -0.5:
                h.Voc_stat_redesign_r_dis[i] = None
            elif h.Ib[i] < 0.5:
                h.Voc_stat_redesign_r_chg[i] = None

    return h


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

        # Save these

        # User inputs
        input_files = ['hist 20220917d-1.txt', '20220917d-20C_sat.txt', 'hist begin30C 20220917d.txt',
                       'hist dead 30C 20220917d.txt', 'hist 20220917d partial 30C chg.txt', 'hist 20230917d 30C chg.txt']
        exclusions = [(1664048488, 1664125892)]  # 30C before full saturation
        # exclusions = None
        data_file = 'data.txt'
        path_to_pdfs = '../dataReduction/figures'
        path_to_data = '../dataReduction'
        path_to_temp = '../dataReduction/temp'
        cols = ('date', 'time', 'Tb', 'Vb', 'Ib', 'soc', 'soc_ekf', 'Voc_dyn', 'Voc_stat', 'tweak_sclr_amp',
                'tweak_sclr_noa', 'falw')

        # cat files
        cat(data_file, input_files, in_path=path_to_data, out_path=path_to_temp)

        # Load mon v4 (old)
        data_file_clean = write_clean_file(data_file, type_='', title_key='hist', unit_key='unit_h',
                                           skip=skip, path_to_data=path_to_temp, path_to_temp=path_to_temp,
                                           comment_str='---')
        if data_file_clean:
            h_raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols, dtype=None,
                                  encoding=None).view(np.recarray)
        else:
            print("data from", data_file, "empty after loading")
            exit(1)

        # Sort unique
        h_raw = np.unique(h_raw)
        print(h_raw)
        # Rack and stack
        if exclusions:
            for i in range(len(exclusions)):
                test_res0 = np.where(h_raw.time < exclusions[i][0])
                test_res1 = np.where(h_raw.time > exclusions[i][1])
                h_raw = h_raw[np.hstack((test_res0, test_res1))[0]]
        h = add_stuff(h_raw, lut_voc, ib_band=IB_BAND)
        voc_soc05 = look_it(x0, lut_voc, 5.)
        h_05C = filter_Tb(h, 5., tb_band=TB_BAND, rated_batt_cap=RATED_BATT_CAP)
        voc_soc11 = look_it(x0, lut_voc, 11.1)
        h_11C = filter_Tb(h, 11.1, tb_band=TB_BAND, rated_batt_cap=RATED_BATT_CAP)
        voc_soc20 = look_it(x0, lut_voc, 20.)
        h_20C = filter_Tb(h, 20., tb_band=TB_BAND, rated_batt_cap=RATED_BATT_CAP)
        voc_soc30 = look_it(x0, lut_voc, 30.)
        h_30C = filter_Tb(h, 30., tb_band=TB_BAND, rated_batt_cap=RATED_BATT_CAP)
        voc_soc40 = look_it(x0, lut_voc, 40.)
        h_40C = filter_Tb(h, 40., tb_band=TB_BAND, rated_batt_cap=RATED_BATT_CAP)

        # Plots
        n_fig = 0
        fig_files = []
        data_root = data_file_clean.split('/')[-1].replace('.csv', '-')
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        if len(h_05C.time) > 1:
            n_fig, fig_files = over_easy(h_05C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_05C', n_fig=n_fig, x_sch=x0, z_sch=voc_soc05, voc_reset=VOC_RESET_05)
        if len(h_11C.time) > 1:
            n_fig, fig_files = over_easy(h_11C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_11C', n_fig=n_fig, x_sch=x0, z_sch=voc_soc11, voc_reset=VOC_RESET_11)
        if len(h_20C.time) > 1:
            n_fig, fig_files = over_easy(h_20C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_20C', n_fig=n_fig, x_sch=x0, z_sch=voc_soc20, voc_reset=VOC_RESET_20)
        if len(h_30C.time) > 1:
            n_fig, fig_files = over_easy(h_30C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_30C', n_fig=n_fig, x_sch=x0, z_sch=voc_soc30, voc_reset=VOC_RESET_30)
        if len(h_40C.time) > 1:
            n_fig, fig_files = over_easy(h_40C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_40C', n_fig=n_fig, x_sch=x0, z_sch=voc_soc40, voc_reset=VOC_RESET_40)
        precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=path_to_pdfs)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo=path_to_pdfs)
        cleanup_fig_files(fig_files)

        plt.show()

    main()
