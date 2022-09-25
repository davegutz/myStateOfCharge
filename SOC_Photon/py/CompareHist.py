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

BATT_RATED_TEMP = 25.  # Temperature at RATED_BATT_CAP, deg C
BATT_V_SAT = 13.8
BATT_DQDT = 0.01  # Change of charge with temperature, fraction / deg C (0.01 from literature)
BATT_DVOC_DT = 0.004  # Change of VOC with operating temperature in range 0 - 50 C V / deg C
RATED_BATT_CAP = 108.4  # A-hr capacity of test article


# Unix-like cat function
# e.g. > cat('out', ['in0', 'in1'], path_to_in='./')
def cat(out_file_name, in_file_names, in_path='./', out_path='./'):
    with open(out_path+'./'+out_file_name, 'w') as out_file:
        for in_file_name in in_file_names:
            with open(in_path+'/'+in_file_name) as in_file:
                for line in in_file:
                    if line.strip():
                        out_file.write(line)


def overall_hist(hi, filename, fig_files=None, plot_title=None, n_fig=None):
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

    plt.figure()  # 1
    n_fig += 1
    plt.subplot(321)
    plt.title(plot_title)
    plt.plot(hi.time, hi.soc, marker='.', markersize='3', linestyle='None', color='black', label='soc')
    plt.plot(hi.time, hi.soc_ekf, marker='.', markersize='3', linestyle='None', color='green', label='soc_ekf')
    plt.legend(loc=1)
    plt.subplot(322)
    plt.plot(hi.time, hi.Tb, marker='.', markersize='3', linestyle='None', color='black', label='Tb')
    plt.plot(hi.time, hi.Vb, marker='.', markersize='3', linestyle='None', color='red', label='Vb')
    plt.plot(hi.time, hi.Voc_dyn, marker='.', markersize='3', linestyle='None', color='blue', label='Voc_dyn')
    plt.plot(hi.time, hi.Voc_stat, marker='.', markersize='3', linestyle='None', color='green', label='Voc_stat')
    plt.legend(loc=1)
    plt.subplot(323)
    plt.plot(hi.time, hi.Ib, marker='+', markersize='3', linestyle='None', color='green', label='Ib')
    plt.legend(loc=1)
    plt.subplot(324)
    plt.plot(hi.time, hi.tweak_sclr_amp, marker='+', markersize='3', linestyle='None', color='orange', label='tweak_sclr_amp')
    plt.plot(hi.time, hi.tweak_sclr_noa, marker='^', markersize='3', linestyle='None', color='green', label='tweak_sclr_noa')
    plt.ylim(-6, 6)
    plt.legend(loc=1)
    plt.subplot(325)
    plt.plot(hi.time, hi.falw, marker='+', markersize='3', linestyle='None', color='magenta', label='falw')
    plt.legend(loc=1)
    plt.subplot(326)
    plt.plot(hi.soc, hi.soc_ekf, marker='+', markersize='3', linestyle='None', color='magenta', label='soc_ekf')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 2
    n_fig += 1
    plt.subplot(221)
    plt.plot(hi.time, hi.Vsat, marker='^', markersize='3', linestyle='None', color='red', label='Vsat')
    plt.plot(hi.time, hi.Vb, marker='1', markersize='3', linestyle='None', color='black', label='Vb')
    plt.plot(hi.time, hi.Voc_dyn, marker='.', markersize='3', linestyle='None', color='orange', label='Voc_dyn')
    plt.plot(hi.time, hi.Voc_stat, marker='_', markersize='3', linestyle='None', color='green', label='Voc_stat')
    plt.plot(hi.time, hi.voc_soc, marker='2', markersize='3', linestyle='None', color='cyan', label='voc_soc')
    plt.legend(loc=1)
    plt.subplot(122)
    plt.plot(hi.time, hi.dscn_fa + 10, marker='o', markersize='3', linestyle='None', color='black', label='dscn_fa+10')
    plt.plot(hi.time, hi.ib_diff_fa + 8, marker='^', markersize='3', linestyle='None', color='blue', label='ib_diff_fa+8')
    plt.plot(hi.time, hi.wv_fa + 7, marker='s', markersize='3', linestyle='None', color='cyan', label='wrap_vb_fa+7')
    plt.plot(hi.time, hi.wl_fa + 6, marker='p', markersize='3', linestyle='None', color='orange', label='wrap_lo_fa+6')
    plt.plot(hi.time, hi.wh_fa + 5, marker='h', markersize='3', linestyle='None', color='green', label='wrap_hi_fa+5')
    plt.plot(hi.time, hi.ccd_fa + 4, marker='H', markersize='3', linestyle='None', color='blue', label='cc_diff_fa+4')
    plt.plot(hi.time, hi.ib_noa_fa + 3, marker='+', markersize='3', linestyle='None', color='red', label='ib_noa_fa+3')
    plt.plot(hi.time, hi.ib_amp_fa + 2, marker='_', markersize='3', linestyle='None', color='magenta', label='ib_amp_fa+2')
    plt.plot(hi.time, hi.vb_fa + 1, marker='1', markersize='3', linestyle='None', color='cyan', label='vb_fa+1')
    plt.plot(hi.time, hi.tb_fa, marker='2', markersize='3', linestyle='None', color='orange', label='tb_fa')
    plt.legend(loc=1)
    plt.subplot(223)
    plt.plot(hi.time, hi.Ib, marker='.', markersize='3', linestyle='None', color='red', label='Ib')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 3
    n_fig += 1
    plt.subplot(111)
    plt.plot(hi.soc, hi.Voc_stat, marker='3', markersize='3', linestyle='None', color='magenta', label='Voc_stat')
    plt.plot(hi.soc, hi.voc_soc, marker='_', markersize='2', linestyle='None', color='black', label='Schedule')
    plt.legend(loc=1)
    plt.title(plot_title)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 4
    n_fig += 1
    plt.subplot(111)
    plt.plot(hi.soc, hi.Voc_stat, marker=0, markersize='3', linestyle='None', color='red', label='Voc_stat')
    plt.plot(hi.soc, hi.voc_soc, marker='_', markersize='2', linestyle='None', color='black', label='Schedule')
    plt.legend(loc=1)
    plt.title(plot_title)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")
    plt.ylim(10, 13.5)
    return n_fig, fig_files


def over_easy(hi, filename, fig_files=None, plot_title=None, n_fig=None, subtitle=None,  x_sch=None, z_sch=None):
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

    plt.figure()  # 1
    n_fig += 1
    plt.subplot(321)
    plt.title(plot_title)
    plt.suptitle(subtitle)
    plt.plot(hi.time, hi.soc, marker='.', markersize='3', linestyle='None', color='black', label='soc')
    plt.plot(hi.time, hi.soc_ekf, marker='.', markersize='3', linestyle='None', color='green', label='soc_ekf')
    plt.legend(loc=1)
    plt.subplot(322)
    plt.plot(hi.time, hi.Tb, marker='.', markersize='3', linestyle='None', color='black', label='Tb')
    plt.plot(hi.time, hi.Vb, marker='.', markersize='3', linestyle='None', color='red', label='Vb')
    plt.plot(hi.time, hi.Voc_dyn, marker='.', markersize='3', linestyle='None', color='blue', label='Voc_dyn')
    plt.plot(hi.time, hi.Voc_stat_chg, marker='.', markersize='3', linestyle='None', color='green', label='Voc_stat_chg')
    plt.plot(hi.time, hi.Voc_stat_dis, marker='.', markersize='3', linestyle='None', color='red', label='Voc_stat_dis')
    plt.legend(loc=1)
    plt.subplot(323)
    plt.plot(hi.time, hi.Ib, marker='+', markersize='3', linestyle='None', color='green', label='Ib')
    plt.legend(loc=1)
    plt.subplot(324)
    plt.plot(hi.time, hi.tweak_sclr_amp, marker='+', markersize='3', linestyle='None', color='orange', label='tweak_sclr_amp')
    plt.plot(hi.time, hi.tweak_sclr_noa, marker='^', markersize='3', linestyle='None', color='green', label='tweak_sclr_noa')
    plt.ylim(-6, 6)
    plt.legend(loc=1)
    plt.subplot(325)
    plt.plot(hi.time, hi.falw, marker='+', markersize='3', linestyle='None', color='magenta', label='falw')
    plt.legend(loc=1)
    plt.subplot(326)
    plt.plot(hi.soc, hi.soc_ekf, marker='+', markersize='3', linestyle='None', color='magenta', label='soc_ekf')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 2
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title)
    plt.suptitle(subtitle)
    plt.plot(hi.time, hi.Vsat, marker='^', markersize='3', linestyle='None', color='red', label='Vsat')
    plt.plot(hi.time, hi.Vb, marker='1', markersize='3', linestyle='None', color='black', label='Vb')
    plt.plot(hi.time, hi.Voc_dyn, marker='.', markersize='3', linestyle='None', color='orange', label='Voc_dyn')
    plt.plot(hi.time, hi.Voc_stat_chg, marker='.', markersize='3', linestyle='None', color='green', label='Voc_stat_chg')
    plt.plot(hi.time, hi.Voc_stat_dis, marker='.', markersize='3', linestyle='None', color='red', label='Voc_stat_dis')
    plt.plot(hi.time, hi.voc_soc, marker='2', markersize='3', linestyle='None', color='cyan', label='voc_soc')
    plt.legend(loc=1)
    plt.subplot(122)
    plt.plot(hi.time, hi.dscn_fa + 18, marker='o', markersize='3', linestyle='None', color='black', label='dscn_fa+18')
    plt.plot(hi.time, hi.ib_diff_fa + 16, marker='^', markersize='3', linestyle='None', color='blue', label='ib_diff_fa+16')
    plt.plot(hi.time, hi.wv_fa + 14, marker='s', markersize='3', linestyle='None', color='cyan', label='wrap_vb_fa+14')
    plt.plot(hi.time, hi.wl_fa + 12, marker='p', markersize='3', linestyle='None', color='orange', label='wrap_lo_fa+12')
    plt.plot(hi.time, hi.wh_fa + 10, marker='h', markersize='3', linestyle='None', color='green', label='wrap_hi_fa+10')
    plt.plot(hi.time, hi.ccd_fa + 8, marker='H', markersize='3', linestyle='None', color='blue', label='cc_diff_fa+8')
    plt.plot(hi.time, hi.ib_noa_fa + 6, marker='+', markersize='3', linestyle='None', color='red', label='ib_noa_fa+6')
    plt.plot(hi.time, hi.ib_amp_fa + 4, marker='_', markersize='3', linestyle='None', color='magenta', label='ib_amp_fa+4')
    plt.plot(hi.time, hi.vb_fa + 2, marker='1', markersize='3', linestyle='None', color='cyan', label='vb_fa+2')
    plt.plot(hi.time, hi.tb_fa, marker='2', markersize='3', linestyle='None', color='orange', label='tb_fa')
    plt.legend(loc=1)
    plt.subplot(223)
    plt.plot(hi.time, hi.Ib, marker='.', markersize='3', linestyle='None', color='red', label='Ib')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 3
    n_fig += 1
    plt.subplot(111)
    plt.title(plot_title)
    plt.suptitle(subtitle)
    plt.plot(hi.soc_r, hi.Voc_stat_r_dis, marker='o', markersize='3', linestyle='-', color='orange', label='Voc_stat_r_dis')
    plt.plot(hi.soc_r, hi.Voc_stat_r_chg, marker='o', markersize='3', linestyle='-', color='cyan', label='Voc_stat_r_chg')
    plt.plot(hi.soc, hi.Voc_stat_dis, marker='.', markersize='3', linestyle='None', color='red', label='Voc_stat_dis')
    plt.plot(hi.soc, hi.Voc_stat_chg, marker='.', markersize='3', linestyle='None', color='green', label='Voc_stat_chg')
    plt.plot(x_sch, z_sch, marker='+', markersize='2', linestyle='--', color='black', label='Schedule')
    plt.legend(loc=4)
    plt.title(plot_title)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")
    plt.ylim(10, 13.5)
    return n_fig, fig_files


# Add schedule lookups and do some rack and stack
def add_stuff(d_ra, voc_soc_tbl):
    voc_soc = []
    Vsat = []
    for i in range(len(d_ra.time)):
        voc_soc.append(voc_soc_tbl.interp(d_ra.soc[i], d_ra.Tb[i]))
        Vsat.append(BATT_V_SAT + (d_ra.Tb[i] - BATT_RATED_TEMP) * BATT_DVOC_DT)
        # np.
    d_mod = rf.rec_append_fields(d_ra, 'voc_soc', np.array(voc_soc, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'Vsat', np.array(voc_soc, dtype=float))
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
        if d_mod.Ib[i] > -0.5:
            Voc_stat_dis[i] = None
        elif d_mod.Ib[i] < 0.5:
            Voc_stat_chg[i] = None
    d_mod = rf.rec_append_fields(d_mod, 'Voc_stat_chg', np.array(Voc_stat_chg, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'Voc_stat_dis', np.array(Voc_stat_dis, dtype=float))
    return d_mod


def calculate_capacity(q_cap_rated_scaled=None, dqdt=None, temp=None, t_rated=None):
    q_cap = q_cap_rated_scaled * (1. + dqdt * (temp - t_rated))
    return q_cap


# Make an array useful for analysis (around temp) and add some metrics
def filter_Tb(raw, temp_corr, vb_band=5., rated_batt_cap=100.):
    h = raw[abs(raw.Tb-temp_corr) < vb_band]

    # Correct for temp
    q_cap = calculate_capacity(q_cap_rated_scaled=rated_batt_cap * 3600., dqdt=BATT_DQDT, temp=h.Tb, t_rated=BATT_RATED_TEMP)
    dq = (h.soc - 1.) * q_cap
    dq -= BATT_DQDT * q_cap * (temp_corr - h.Tb)
    q_cap_r = calculate_capacity(q_cap_rated_scaled=rated_batt_cap * 3600., dqdt=BATT_DQDT, temp=temp_corr, t_rated=BATT_RATED_TEMP)
    h.soc_r = 1. + dq / q_cap_r
    h.Voc_stat_r = h.Voc_stat - (h.Tb - temp_corr) * BATT_DVOC_DT

    # delineate charging and discharging
    h.Voc_stat_r_chg = np.copy(h.Voc_stat)
    h.Voc_stat_r_dis = np.copy(h.Voc_stat)
    for i in range(len(h.Voc_stat_r_chg)):
        if h.Ib[i] > -0.5:
            h.Voc_stat_r_dis[i] = None
        elif h.Ib[i] < 0.5:
            h.Voc_stat_r_chg[i] = None

    return h


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
        input_files = ['hist 20220917d-1.txt', '20220917d-20C_sat.txt']
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
        h = add_stuff(h_raw, lut_voc)
        h_11C = filter_Tb(h, 11.1, vb_band=5., rated_batt_cap=RATED_BATT_CAP)
        x_soc = x0; voc_soc20 = x0.copy()
        for i in range(len(x_soc)):
            voc_soc20[i] = lut_voc.interp(x_soc[i], 20.)
        h_20C = filter_Tb(h, 20., vb_band=5., rated_batt_cap=RATED_BATT_CAP)
        h_30C = filter_Tb(h, 30., vb_band=5., rated_batt_cap=RATED_BATT_CAP)
        h_40C = filter_Tb(h, 40., vb_band=5., rated_batt_cap=RATED_BATT_CAP)

        # Plots
        n_fig = 0
        fig_files = []
        data_root = data_file_clean.split('/')[-1].replace('.csv', '-')
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        # n_fig, fig_files = over_easy(h_11C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_11C',
        #                              n_fig=n_fig, x_sch=x_soc, z_sch=voc_soc20)
        n_fig, fig_files = over_easy(h_20C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_20C',
                                     n_fig=n_fig, x_sch=x_soc, z_sch=voc_soc20)
        # n_fig, fig_files = over_easy(h_30C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_30C',
        #                              n_fig=n_fig, x_sch=x_soc, z_sch=voc_soc20)
        # n_fig, fig_files = over_easy(h_40C, filename, fig_files=fig_files, plot_title=plot_title, subtitle='h_40C',
        #                              n_fig=n_fig, x_sch=x_soc, z_sch=voc_soc20)
        precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=path_to_pdfs)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo=path_to_pdfs)
        cleanup_fig_files(fig_files)

        plt.show()

    main()
