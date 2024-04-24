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

import numpy as np
import numpy.lib.recfunctions as rf
import matplotlib.pyplot as plt
from Hysteresis_20220917d import Hysteresis_20220917d
from Hysteresis_20220926 import Hysteresis_20220926
from Battery import Battery, BatteryMonitor, is_sat, Retained
from MonSim import replicate, save_clean_file
# from Util import cat
from resample import resample
from PlotKiller import show_killer
from DataOverModel import dom_plot
from PlotGP import tune_r, gp_plot
from PlotOffOn import off_on_plot
from Chemistry_BMS import ib_lag
from myFilters import LagExp
import sys
from DataOverModel import write_clean_file
from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
from datetime import datetime
import os
from load_data import load_data, remove_nan
import tkinter.messagebox

plt.rcParams['axes.grid'] = True

#  For this battery Battleborn 100 Ah with 1.084 x capacity
IB_BAND = 1.  # Threshold to declare charging or discharging
TB_BAND = 25.  # Band around temperature to group data and correct
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
HYS_SOC_MIN_MARG = 0.15  # add to soc_min to set thr for detecting low endpoint condition for reset of hysteresis


# Add schedule lookups and do some rack and stack
def add_stuff(d_ra, mon, ib_band=0.5):
    voc_soc = []
    soc_min = []
    vsat = []
    time_sec = []
    dt = []
    for i in range(len(d_ra.time_ux)):
        voc_soc.append(mon.chemistry.lut_voc_soc.interp(d_ra.soc[i], d_ra.Tb[i]))
        soc_min.append((mon.chemistry.lut_min_soc.interp(d_ra.Tb[i])))
        vsat.append(mon.chemistry.nom_vsat + (d_ra.Tb[i] - mon.chemistry.rated_temp) * mon.chemistry.dvoc_dt)
        time_sec.append(float(d_ra.time_ux[i] - d_ra.time_ux[0]))
        if i > 0:
            dt.append(float(d_ra.time_ux[i] - d_ra.time_ux[i - 1]))
        else:
            dt.append(float(d_ra.time_ux[1] - d_ra.time_ux[0]))
    time_min = (d_ra.time_ux-d_ra.time_ux[0])/60.
    time_day = (d_ra.time_ux-d_ra.time_ux[0])/3600./24.
    d_mod = rf.rec_append_fields(d_ra, 'time_sec', np.array(time_sec, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'time', np.array(time_sec, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'time_min', np.array(time_min, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'time_day', np.array(time_day, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'voc_soc', np.array(voc_soc, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'soc_min', np.array(soc_min, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'vsat', np.array(vsat, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'ib_sel', np.array(d_mod.ib, dtype=float))
    if hasattr(d_mod, 'voc_dyn'):
        voc = d_mod.voc_dyn.copy()
        d_mod = rf.rec_append_fields(d_mod, 'voc', np.array(voc, dtype=float))
    d_mod = calc_fault(d_ra, d_mod)
    voc_stat_chg = np.copy(d_mod.voc_stat)
    voc_stat_dis = np.copy(d_mod.voc_stat)
    for i in range(len(voc_stat_chg)):
        if d_mod.ib[i] > -ib_band:
            voc_stat_dis[i] = None
        elif d_mod.ib[i] < ib_band:
            voc_stat_chg[i] = None
    d_mod = rf.rec_append_fields(d_mod, 'voc_stat_chg', np.array(voc_stat_chg, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'voc_stat_dis', np.array(voc_stat_dis, dtype=float))
    if hasattr(d_mod, 'voc_dyn'):
        dv_hys = d_mod.voc_dyn - d_mod.voc_stat
    else:
        dv_hys = d_mod.voc - d_mod.voc_stat
    d_mod = rf.rec_append_fields(d_mod, 'dv_hys', np.array(dv_hys, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'dV_hys', np.array(dv_hys, dtype=float))
    dv_hys_unscaled = d_mod.dv_hys / HYS_SCALE_20220917d
    d_mod = rf.rec_append_fields(d_mod, 'dv_hys_unscaled', np.array(dv_hys_unscaled, dtype=float))
    if hasattr(d_mod, 'voc_dyn'):
        dv_hys_required = d_mod.voc_dyn - voc_soc + dv_hys
    else:
        dv_hys_required = d_mod.voc - voc_soc + dv_hys
    d_mod = rf.rec_append_fields(d_mod, 'dv_hys_required', np.array(dv_hys_required, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'dt', np.array(dt, dtype=float))

    dv_hys_rescaled = d_mod.dv_hys_unscaled
    pos = dv_hys_rescaled >= 0
    neg = dv_hys_rescaled < 0
    dv_hys_rescaled[pos] *= HYS_RESCALE_CHG
    dv_hys_rescaled[neg] *= HYS_RESCALE_DIS
    d_mod = rf.rec_append_fields(d_mod, 'dv_hys_rescaled', np.array(dv_hys_rescaled, dtype=float))
    if hasattr(d_mod, 'voc_dyn'):
        voc_stat_rescaled = d_mod.voc_dyn - d_mod.dv_hys_rescaled
    else:
        voc_stat_rescaled = d_mod.voc - d_mod.dv_hys_rescaled
    d_mod = rf.rec_append_fields(d_mod, 'voc_stat_rescaled', np.array(voc_stat_rescaled, dtype=float))

    return d_mod


# Add ib_lag = ib lagged by time constant
def add_ib_lag(data, mon):
    lag_tau = ib_lag(mon.chemistry.mod_code)
    IbLag = LagExp(1., lag_tau, -100., 100.)
    n = len(data.time)
    if n < 2:
        return data
    if hasattr(data, 'ib_lag') is False:
        data = rf.rec_append_fields(data, 'ib_lag', np.array(data.time, dtype=float))
        data.ib_lag = np.zeros(n)
    dt = data.time[1] - data.time[0]
    for i in range(n):
        if i > 0:
            dt = data.time[i] - data.time[i-1]
        data.ib_lag[i] = IbLag.calculate_tau(float(data.ib[i]), i == 0, dt, lag_tau)
    return data


# Add schedule lookups and do some rack and stack
def add_stuff_f(d_ra, mon, ib_band=0.5, rated_batt_cap=100., Dw=0., time_sync=None, unit=None):
    voc_soc = []
    soc_min = []
    vsat = []
    time_sec = []
    cc_diff_thr = []
    cc_dif = []
    ewhi_thr = []
    ewlo_thr = []
    ib_diff_thr = []
    ib_quiet_thr = []
    ib_diff = []
    dt = []
    ib_charge = []
    dv_dyn = []
    bms_off_init = False
    bms_off = False
    rp = Retained()
    if time_sync is None:
        time_sync = d_ra.time_ux[0]
    for i in range(len(d_ra.time_ux)):
        soc = d_ra.soc[i]
        voc_stat = d_ra.voc_stat[i]
        Tb = d_ra.Tb[i]
        ib_diff_ = d_ra.ibmh[i] - d_ra.ibnh[i]
        cc_dif_ = d_ra.soc[i] - d_ra.soc_ekf[i]
        ib_diff.append(ib_diff_)
        C_rate = d_ra.ib[i] / rated_batt_cap
        voc_soc.append(mon.chemistry.lut_voc_soc.interp(d_ra.soc[i], d_ra.Tb[i]) + Dw)
        BB = BatteryMonitor(0, unit=unit)
        cc_diff_thr_, ewhi_thr_, ewlo_thr_, ib_diff_thr_, ib_quiet_thr_ = \
            fault_thr_bb(Tb, soc, voc_soc[i], voc_stat, C_rate, BB)
        ib_ = d_ra.ib[i]
        temp_c = d_ra.Tb[i]
        vb_ = d_ra.vb[i]
        voc_ = d_ra.voc[i]
        reset = i == 0
        # Battery management system model (uses past value bms_off and voc_stat)
        if not bms_off:
            voltage_low = voc_stat < mon.chemistry.vb_down
        else:
            voltage_low = voc_stat < mon.chemistry.vb_rising
        bms_charging = ib_ > Battery.IB_MIN_UP
        if reset and bms_off_init is not None:
            bms_off = bms_off_init
        else:
            bms_off = (temp_c <= mon.chemistry.low_t) or (voltage_low and not rp.tweak_test())  # KISS
        ib_charge_ = ib_
        if bms_off and not bms_charging:
            ib_charge_ = 0.
        cc_dif.append(cc_dif_)
        cc_diff_thr.append(cc_diff_thr_)
        ewhi_thr.append(ewhi_thr_)
        ewlo_thr.append(ewlo_thr_)
        ib_diff_thr.append(ib_diff_thr_)
        ib_quiet_thr.append(ib_quiet_thr_)
        soc_min.append((BB.chemistry.lut_min_soc.interp(d_ra.Tb[i])))
        vsat.append(mon.chemistry.nom_vsat + (d_ra.Tb[i] - mon.chemistry.rated_temp) * mon.chemistry.dvoc_dt)
        time_sec.append(float(d_ra.time_ux[i] - time_sync))
        if i > 0:
            dt.append(float(d_ra.time_ux[i] - d_ra.time_ux[i - 1]))
        elif len(d_ra.time_ux) > 1:
            dt.append(float(d_ra.time_ux[1] - d_ra.time_ux[0]))
        else:
            pass
        dv_dyn_ = vb_ - voc_
        ib_charge.append(ib_charge_)
        dv_dyn.append(dv_dyn_)
    time_min = (d_ra.time_ux - time_sync)/60.
    time_day = (d_ra.time_ux - time_sync)/3600./24.
    d_mod = rf.rec_append_fields(d_ra, 'time_sec', np.array(time_sec, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'time', np.array(time_sec, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'time_min', np.array(time_min, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'time_day', np.array(time_day, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'voc_soc', np.array(voc_soc, dtype=float))
    if not hasattr(d_mod, 'soc_min'):
        d_mod = rf.rec_append_fields(d_mod, 'soc_min', np.array(soc_min, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'vsat', np.array(vsat, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'ib_diff', np.array(ib_diff, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'cc_diff_thr', np.array(cc_diff_thr, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'cc_dif', np.array(cc_dif, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'ewhi_thr', np.array(ewhi_thr, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'ewlo_thr', np.array(ewlo_thr, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'ib_diff_thr', np.array(ib_diff_thr, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'ib_quiet_thr', np.array(ib_quiet_thr, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'dt', np.array(dt, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'ib_charge', np.array(ib_charge, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'dv_dyn', np.array(dv_dyn, dtype=float))
    d_mod = add_ib_lag(d_mod, mon)
    d_mod = calc_fault(d_ra, d_mod)
    voc_stat_chg = np.copy(d_mod.voc_stat)
    voc_stat_dis = np.copy(d_mod.voc_stat)
    for i in range(len(voc_stat_chg)):
        if d_mod.ib[i] > -ib_band:
            voc_stat_dis[i] = None
        elif d_mod.ib[i] < ib_band:
            voc_stat_chg[i] = None
    d_mod = rf.rec_append_fields(d_mod, 'voc_stat_chg', np.array(voc_stat_chg, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'voc_stat_dis', np.array(voc_stat_dis, dtype=float))
    dv_hys = d_mod.voc - d_mod.voc_stat
    d_mod = rf.rec_append_fields(d_mod, 'dv_hys', np.array(dv_hys, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'dV_hys', np.array(dv_hys, dtype=float))
    dv_hys_unscaled = d_mod.dv_hys / HYS_SCALE_20220917d
    d_mod = rf.rec_append_fields(d_mod, 'dv_hys_unscaled', np.array(dv_hys_unscaled, dtype=float))
    dv_hys_required = d_mod.voc - voc_soc + dv_hys
    d_mod = rf.rec_append_fields(d_mod, 'dv_hys_required', np.array(dv_hys_required, dtype=float))

    dv_hys_rescaled = d_mod.dv_hys_unscaled
    pos = dv_hys_rescaled >= 0
    neg = dv_hys_rescaled < 0
    dv_hys_rescaled[pos] *= HYS_RESCALE_CHG
    dv_hys_rescaled[neg] *= HYS_RESCALE_DIS
    d_mod = rf.rec_append_fields(d_mod, 'dv_hys_rescaled', np.array(dv_hys_rescaled, dtype=float))
    voc_stat_rescaled = d_mod.voc - d_mod.dv_hys_rescaled
    d_mod = rf.rec_append_fields(d_mod, 'voc_stat_rescaled', np.array(voc_stat_rescaled, dtype=float))

    # vb = d_mod.vb.copy()
    # d_mod = rf.rec_append_fields(d_mod, 'vb', np.array(vb, dtype=float))
    voc_dyn = d_mod.voc.copy()
    d_mod = rf.rec_append_fields(d_mod, 'voc_dyn', np.array(voc_dyn, dtype=float))
    ib = d_mod.ib.copy()
    # d_mod = rf.rec_append_fields(d_mod, 'ib', np.array(ib, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'ib_sel', np.array(ib, dtype=float))
    d_zero = d_mod.ib.copy()*0.
    d_mod = rf.rec_append_fields(d_mod, 'tweak_sclr_amp', np.array(d_zero, dtype=float))
    d_mod = rf.rec_append_fields(d_mod, 'tweak_sclr_noa', np.array(d_zero, dtype=float))

    return d_mod


# Calculate thresholds from global input values listed above (review these)
def fault_thr_bb(Tb, soc, voc_soc, voc_stat, C_rate, bb):
    # There is no fault logic in the python code, so hard code it here
    WRAP_HI_A = 32.  # Wrap high voltage threshold, A (32 after testing; 16=0.2v)
    WRAP_LO_A = -32.  # Wrap high voltage threshold, A (-32, -20 too small on truck -16=-0.2v)
    WRAP_HI_SAT_MARG = 0.2  # Wrap voltage margin to saturation, V (0.2)
    WRAP_HI_SAT_SCLR = 2.0  # Wrap voltage margin scalar when saturated (2.0)
    IBATT_DISAGREE_THRESH = 10.  # Signal selection threshold for current disagree test, A (10.)
    QUIET_A = 0.005  # Quiet set threshold, sec (0.005, 0.01 too large in truck)
    WRAP_SOC_HI_OFF = 0.97  # Disable e_wrap_hi when saturated
    WRAP_SOC_HI_SCLR = 1000.  # Huge to disable e_wrap
    WRAP_SOC_LO_OFF_ABS = 0.35  # Disable e_wrap when near empty (soc lo any Tb)
    WRAP_SOC_LO_OFF_REL = 0.2  # Disable e_wrap when near empty (soc lo for high Tb where soc_min=.2, voltage cutback)
    WRAP_SOC_LO_SCLR = 60.  # Large to disable e_wrap (60. for startup)
    CC_DIFF_SOC_DIS_THRESH = 0.2  # Signal selection threshold for Coulomb counter EKF disagree test (0.2)
    CC_DIFF_LO_SOC_SCLR = 4.  # Large to disable cc_ctf
    WRAP_MOD_C_RATE = .05  # Moderate charge rate threshold to engage wrap threshold 0
    WRAP_SOC_MOD_OFF = 0.85  # Disable e_wrap_lo when nearing saturated and moderate C_rate (0.85)

    vsat_ = bb.chemistry.nom_vsat + (Tb-25.)*bb.chemistry.dvoc_dt

    # cc_diff
    soc_min = bb.chemistry.lut_min_soc.interp(Tb)
    if soc <= max(soc_min+WRAP_SOC_LO_OFF_REL, WRAP_SOC_LO_OFF_ABS):
        cc_diff_empty_sclr_ = CC_DIFF_LO_SOC_SCLR
    else:
        cc_diff_empty_sclr_ = 1.
    cc_diff_sclr_ = 1.  # ram adjusts during data collection
    cc_diff_thr = CC_DIFF_SOC_DIS_THRESH * cc_diff_sclr_ * cc_diff_empty_sclr_

    # wrap
    if soc >= WRAP_SOC_HI_OFF:
        ewsat_sclr_ = WRAP_SOC_HI_SCLR
        ewmin_sclr_ = 1.
    elif soc <= max(soc_min+WRAP_SOC_LO_OFF_REL, WRAP_SOC_LO_OFF_ABS):
        ewsat_sclr_ = 1.
        ewmin_sclr_ = WRAP_SOC_LO_SCLR
    elif voc_soc > (vsat_ - WRAP_HI_SAT_MARG) or \
            (voc_stat > (vsat_ - WRAP_HI_SAT_MARG) and C_rate > WRAP_MOD_C_RATE and soc > WRAP_SOC_MOD_OFF):
        ewsat_sclr_ = WRAP_HI_SAT_SCLR
        ewmin_sclr_ = 1.
    else:
        ewsat_sclr_ = 1.
        ewmin_sclr_ = 1.
    ewhi_sclr_ = 1.  # ram adjusts during data collection
    ewhi_thr = bb.chemistry.r_ss * WRAP_HI_A * ewhi_sclr_ * ewsat_sclr_ * ewmin_sclr_
    ewlo_sclr_ = 1.  # ram adjusts during data collection
    ewlo_thr = bb.chemistry.r_ss * WRAP_LO_A * ewlo_sclr_ * ewsat_sclr_ * ewmin_sclr_

    # ib_diff
    ib_diff_sclr_ = 1.  # ram adjusts during data collection
    ib_diff_thr = IBATT_DISAGREE_THRESH * ib_diff_sclr_

    # ib_quiet
    ib_quiet_sclr_ = 1.  # ram adjusts during data collection
    ib_quiet_thr = QUIET_A * ib_quiet_sclr_

    return cc_diff_thr, ewhi_thr, ewlo_thr, ib_diff_thr, ib_quiet_thr


def over_fault(hi, filename, fig_files=None, plot_title=None, fig_list=None, subtitle=None, long_term=True,
               cc_dif_tol=0.2):
    if fig_files is None:
        fig_files = []

    if long_term:
        fig_list.append(plt.figure())  # 1
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
        plt.plot(hi.time, hi.tweak_sclr_amp, marker='+', markersize='3', linestyle='None', color='orange',
                 label='tweak_sclr_amp')
        plt.plot(hi.time, hi.tweak_sclr_noa, marker='^', markersize='3', linestyle='None', color='green',
                 label='tweak_sclr_noa')
        plt.ylim(-6, 6)
        plt.legend(loc=1)
        plt.subplot(335)
        plt.plot(hi.time, hi.falw, marker='+', markersize='3', linestyle='None', color='magenta', label='falw')
        plt.legend(loc=0)
        plt.subplot(336)
        plt.plot(hi.soc, hi.soc_ekf, marker='+', markersize='3', linestyle='-', color='blue', label='soc_ekf')
        plt.plot([0, 1], [cc_dif_tol, 1.+cc_dif_tol], linestyle=':', color='red')
        plt.plot([0, 1], [-cc_dif_tol, 1.-cc_dif_tol], linestyle=':', color='red')
        plt.xlim(0, 1)
        plt.ylim(0, 1)
        plt.xlabel('soc')
        plt.legend(loc=4)
        plt.subplot(337)
        plt.plot(hi.time, hi.dv_hys, marker='o', markersize='3', linestyle='-', color='blue', label='dv_hys')
        plt.plot(hi.time, hi.dv_hys_rescaled, marker='o', markersize='3', linestyle='-', color='cyan',
                 label='dv_hys_rescaled')
        plt.plot(hi.time, hi.dv_hys_required, linestyle='--', color='black', label='dv_hys_required')
        plt.plot(hi.time, -hi.e_wrap, marker='o', markersize='3', linestyle='None', color='red', label='-e_wrap')
        plt.plot(hi.time, hi.dv_hys_remodel, marker='x', markersize='3', linestyle=':', color='lawngreen',
                 label='dv_hys_remodel')
        plt.plot(hi.time, hi.dv_hys_redesign_chg, marker=3, markersize='3', linestyle=':', color='springgreen',
                 label='dv_hys_redesign_chg')
        plt.plot(hi.time, hi.dv_hys_redesign_dis, marker=3, markersize='3', linestyle=':', color='orangered',
                 label='dv_hys_redesign_dis')
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
        plt.plot(hi.time, hi.voc, marker='.', markersize='3', linestyle='None', color='blue', label='voc')
        plt.plot(hi.time, hi.voc_stat_chg, marker='.', markersize='3', linestyle='None', color='green',
                 label='voc_stat_chg')
        plt.plot(hi.time, hi.voc_stat_dis, marker='.', markersize='3', linestyle='None', color='red',
                 label='voc_stat_dis')
        plt.xlabel('days')
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        fig_list.append(plt.figure())  # 2
        plt.subplot(221)
        plt.title(plot_title + ' f2')
        plt.suptitle(subtitle)
        plt.plot(hi.time, hi.vsat, marker='.', markersize='1', linestyle='-', color='orange', linewidth='1',
                 label='vsat')
        plt.plot(hi.time, hi.vb, marker='1', markersize='3', linestyle='None', color='black', label='vb')
        plt.plot(hi.time, hi.voc, marker='.', markersize='3', linestyle='None', color='orange', label='voc')
        plt.plot(hi.time, hi.voc_stat_chg, marker='.', markersize='3', linestyle='-', color='green',
                 label='voc_stat_chg')
        plt.plot(hi.time, hi.voc_stat_dis, marker='.', markersize='3', linestyle='-', color='red',
                 label='voc_stat_dis')
        plt.plot(hi.time, hi.voc_soc, marker='2', markersize='3', linestyle=':', color='cyan', label='voc_soc')
        plt.xlabel('days')
        plt.legend(loc=1)
        plt.subplot(122)
        plt.plot(hi.time, hi.bms_off + 22, marker='h', markersize='3', linestyle='-', color='blue',
                 label='bms_off+22')
        plt.plot(hi.time, hi.sat + 20, marker='s', markersize='3', linestyle='-', color='red', label='sat+20')
        plt.plot(hi.time, hi.dscn_fa + 18, marker='o', markersize='3', linestyle='-', color='black',
                 label='dscn_fa+18')
        plt.plot(hi.time, hi.ib_diff_fa + 16, marker='^', markersize='3', linestyle='-', color='blue',
                 label='ib_diff_fa+16')
        plt.plot(hi.time, hi.wv_fa + 14, marker='s', markersize='3', linestyle='-', color='cyan',
                 label='wrap_vb_fa+14')
        plt.plot(hi.time, hi.wl_fa + 12, marker='p', markersize='3', linestyle='-', color='orange',
                 label='wrap_lo_fa+12')
        plt.plot(hi.time, hi.wh_fa + 10, marker='h', markersize='3', linestyle='-', color='green',
                 label='wrap_hi_fa+10')
        plt.plot(hi.time, hi.ccd_fa + 8, marker='H', markersize='3', linestyle='-', color='blue',
                 label='cc_diff_fa+8')
        plt.plot(hi.time, hi.ib_noa_fa + 6, marker='+', markersize='3', linestyle='-', color='red',
                 label='ib_noa_fa+6')
        plt.plot(hi.time, hi.ib_amp_fa + 4, marker='_', markersize='3', linestyle='-', color='magenta',
                 label='ib_amp_fa+4')
        plt.plot(hi.time, hi.vb_fa + 2, marker='1', markersize='3', linestyle='-', color='cyan',
                 label='vb_fa+2')
        plt.plot(hi.time, hi.tb_fa, marker='2', markersize='3', linestyle='-', color='orange',
                 label='tb_fa')
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

        fig_list.append(plt.figure())  # 3
        plt.subplot(221)
        plt.title(plot_title + ' f3')
        plt.suptitle(subtitle)
        plt.plot(hi.time, hi.dv_hys, marker='o', markersize='3', linestyle='-', color='blue', label='dv_hys')
        plt.plot(hi.time, hi.dv_hys_rescaled, marker='o', markersize='3', linestyle='-', color='cyan',
                 label='dv_hys_rescaled')
        plt.plot(hi.time, hi.dv_hys_required, linestyle='--', color='black', label='dv_hys_required')
        plt.plot(hi.time, -hi.e_wrap, marker='o', markersize='3', linestyle='None', color='red', label='-e_wrap')
        plt.plot(hi.time, hi.dv_hys_redesign_chg, marker=3, markersize='3', linestyle='-', color='green',
                 label='dv_hys_redesign_chg')
        plt.plot(hi.time, hi.dv_hys_redesign_dis, marker=3, markersize='3', linestyle='-', color='red',
                 label='dv_hys_redesign_dis')
        plt.xlabel('days')
        plt.legend(loc=4)
        # plt.ylim(-0.7, 0.7)
        plt.ylim(bottom=-0.7)
        plt.subplot(222)
        plt.plot(hi.time, hi.res_redesign_chg, marker='o', markersize='3', linestyle='-', color='green',
                 label='res_redesign_chg')
        plt.plot(hi.time, hi.res_redesign_dis, marker='o', markersize='3', linestyle='-', color='red',
                 label='res_redesign_dis')
        plt.xlabel('days')
        plt.legend(loc=4)
        plt.subplot(223)
        plt.plot(hi.time, hi.ib, color='black', label='ib')
        plt.plot(hi.time, hi.soc*10, color='green', label='soc*10')
        plt.plot(hi.time, hi.ioc_redesign, marker='o', markersize='3', linestyle='-', color='cyan',
                 label='ioc_redesign')
        plt.xlabel('days')
        plt.legend(loc=4)
        plt.subplot(224)
        plt.plot(hi.time, hi.dv_dot_redesign, linestyle='--', color='black', label='dv_dot_redesign')
        plt.xlabel('days')
        plt.legend(loc=4)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

    fig_list.append(plt.figure())  # 4
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
    if hasattr(hi, 'e_wrap_filt'):
        plt.plot(hi.time, hi.e_wrap_filt, color='black', linestyle='--', label='e_wrap_filt')
    else:
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
    # plt.ylim(-.01, .01)
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


def overall_fault(mo, mv, sv, smv, filename, fig_files=None, plot_title=None, fig_list=None):
    if fig_files is None:
        fig_files = []

    fig_list.append(plt.figure())  # of 1
    plt.subplot(331)
    plt.title(plot_title + ' O_F 1')
    plt.plot(mo.time, mo.ib_sel, color='black', linestyle='-', label='ib_sel=ib_in')
    plt.plot(mv.time, mv.ib_in, color='cyan', linestyle='--', label='ib_in_ver')
    plt.plot(smv.time, smv.ib_in_s, color='orange', linestyle='-.', label='ib_in_s_ver')
    plt.plot(mo.time, mo.Tb, color='red', linestyle='-', label='temp_c')
    plt.plot(mv.time, mv.Tb, color='blue', linestyle='--', label='temp_c_ver')
    plt.plot(smv.time, mv.Tb, color='green', linestyle='-.', label='temp_c_s_ver')
    plt.legend(loc=1)
    plt.subplot(332)
    if hasattr(mo, 'ioc'):
        plt.plot(mo.time, mo.ioc, color='black', linestyle='-', label='ioc')
    plt.plot(mv.time, mv.ioc, color='cyan', linestyle='--', label='ioc_ver')
    plt.plot(sv.time, sv.ioc, color='orange', linestyle=':', label='ioc_s_ver')
    plt.legend(loc=1)
    plt.subplot(333)
    plt.plot(mo.time, mo.dV_hys, color='black', linestyle='-', label='dV_hys')
    plt.plot(mv.time, mv.dv_hys, color='cyan', linestyle='--', label='dv_hys_ver')
    plt.plot(sv.time, sv.dv_hys, color='orange', linestyle='-.', label='dv_hys_s_ver')
    plt.legend(loc=1)
    plt.subplot(334)
    plt.plot(mo.time, mo.vb, color='black', linestyle='-', label='vb')
    plt.plot(mv.time, mv.vb, color='cyan', linestyle='--', label='vb_ver')
    plt.plot(smv.time, smv.vb_s, color='orange', linestyle='-.', label='vb_s_ver')
    plt.legend(loc=1)
    plt.subplot(335)
    plt.plot(mo.time, mo.voc, color='black', linestyle='-', label='voc')
    plt.plot(mv.time, mv.voc, color='cyan', linestyle='--', label='voc_ver')
    plt.plot(smv.time, smv.voc_s, color='orange', linestyle='-.', label='voc_s_ver')
    plt.legend(loc=1)
    plt.subplot(336)
    plt.plot(mo.time, mo.soc, color='black', linestyle='-', label='soc')
    plt.plot(mv.time, mv.soc, color='cyan', linestyle='--', label='soc_ver')
    plt.plot(smv.time, smv.soc_s, color='orange', linestyle='-.', label='soc_s_ver')
    plt.plot(mo.time, mo.soc_ekf, color='blue', linestyle='-', label='soc_ekf')
    plt.plot(mv.time, mv.soc_ekf, color='red', linestyle='--', label='soc_ekf_ver')
    plt.legend(loc=1)
    plt.subplot(337)
    plt.plot(mo.time, mo.voc_stat, color='black', linestyle='-', label='voc_stat')
    plt.plot(mv.time, mv.voc_stat, color='cyan', linestyle='--', label='voc_stat_ver')
    plt.plot(smv.time, smv.voc_stat_s, color='orange', linestyle='-.', label='voc_stat_s_ver')
    plt.legend(loc=1)
    plt.subplot(338)
    plt.plot(mo.time, mo.e_wrap, color='black', linestyle='-', label='e_wrap')
    plt.plot(mv.time, np.array(mv.voc_soc) - np.array(mv.voc_stat), color='cyan', linestyle='--',
             label='e_wrap_ver')
    plt.plot(mo.time, np.array(mo.soc_ekf) - np.array(mo.soc), color='blue', linestyle='-', label='cc_dif')
    plt.plot(mv.time, np.array(mv.soc_ekf) - np.array(mv.soc), color='red', linestyle='--', label='cc_dif_ver')
    plt.plot(mo.time, mo.cc_diff_thr, color='cyan', linestyle='--', label='cc_diff_thr')
    plt.plot(mo.time, -mo.cc_diff_thr, color='cyan', linestyle='--')
    # plt.plot(smv.time, np.array(smv.voc_soc_s) - np.array(smv.voc_stat_s), color='orange', linestyle='-.',
    # label='e_wrap_filt_s_ver')
    if hasattr(mo, 'ewhi_thr'):
        plt.plot(mo.time, mo.ewhi_thr, color='red', linestyle='-.', label='ewhi_thr')
    if hasattr(mo, 'ewlo_thr'):
        plt.plot(mo.time, mo.ewlo_thr, color='red', linestyle='-.', label='ewlo_thr')
    plt.ylim(-.5, .5)
    plt.legend(loc=1)
    plt.subplot(339)
    if hasattr(mo, 'dv_dyn'):
        plt.plot(mo.time, mo.dv_dyn, color='black', linestyle='-', label='dv_dyn')
    plt.plot(mv.time, mv.dv_dyn, color='cyan', linestyle='--', label='dv_dyn_ver')
    plt.plot(smv.time, smv.dv_dyn_s, color='orange', linestyle='-.', label='dv_dyn_s_ver')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    ref_str = ''
    test_str = '_ver'

    fig_list.append(plt.figure())  # GP 2 Hist
    plt.subplot(331)
    plt.title(plot_title + ' GP 2 Hist')
    mo.dv_dyn = mo.vb - mo.voc
    plt.plot(mo.time, mo.dv_dyn, color='blue', linestyle='-', label='dv_dyn'+ref_str)
    plt.plot(mv.time, mv.dv_dyn, color='cyan', linestyle='--', label='dv_dyn'+test_str)
    # plt.plot(so.time, so.dv_dyn_s, color='black', linestyle='-.', label='dv_dyn_s'+ref_str)
    plt.plot(mv.time, smv.dv_dyn_s, color='magenta', linestyle=':', label='dv_dyn_s'+test_str)
    plt.xlabel('sec')
    plt.legend(loc=3)
    plt.subplot(332)
    plt.plot(mo.time, mo.soc, linestyle='-', color='blue', label='soc'+ref_str)
    plt.plot(mv.time, mv.soc, linestyle='--', color='cyan', label='soc'+test_str)
    # plt.plot(so.time, so.soc_s, linestyle='-.', color='black', label='soc_s'+ref_str)
    plt.plot(sv.time, smv.soc_s, linestyle=':', color='magenta', label='soc_s'+test_str)
    plt.plot(mo.time, mo.soc_ekf, linestyle='-', color='green', label='soc_ekf'+ref_str)
    plt.plot(mv.time, mv.soc_ekf, linestyle='--', color='red', label='soc_ekf'+test_str)
    plt.xlabel('sec')
    plt.legend(loc=4)
    plt.subplot(333)
    plt.plot(mo.time, mo.ib_sel, linestyle='-', color='blue', label='ib_sel'+ref_str)
    plt.plot(mv.time, mv.ib_in, linestyle='-', color='cyan', label='ib_in'+test_str)
    # plt.plot(so.time, so.ib_s, linestyle='--', color='black', label='ib_in_s'+ref_str)
    plt.plot(smv.time, smv.ib_in_s, linestyle=':', color='red', label='ib_in_s'+test_str)
    plt.xlabel('sec')
    plt.legend(loc=3)
    plt.subplot(334)
    plt.plot(mo.time, mo.voc, linestyle='-', color='blue', label='voc'+ref_str)
    plt.plot(mv.time, mv.voc, linestyle='--', color='cyan', label='voc'+test_str)
    # plt.plot(so.time, so.voc_s, linestyle='-', color='blue', label='voc_s'+ref_str)
    # plt.plot(smv.time, smv.voc_s, linestyle='--', color='cyan', label='voc_s'+test_str)
    # plt.plot(so.time, so.voc_stat_s, linestyle='-.', color='blue', label='voc_stat_s'+ref_str)
    plt.plot(mo.time, mo.voc_stat, color='orange', linestyle='-', label='voc_stat'+ref_str)
    plt.plot(smv.time, smv.voc_stat_s, linestyle=':', color='red', label='voc_stat_s'+test_str)
    # plt.plot(so.time, so.vb_s, linestyle='-', color='orange', label='vb_s'+ref_str)
    plt.plot(sv.time, smv.vb_s, linestyle='--', color='pink', label='vb_s'+test_str)
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(335)
    plt.plot(mo.time, mo.e_wrap, color='black', linestyle='-', label='e_wrap'+ref_str)
    plt.plot(mv.time, mv.e_wrap, color='orange', linestyle='--', label='e_wrap'+test_str)
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(336)
    plt.plot(mo.soc, mo.vb, color='blue', linestyle='-', label='vb'+ref_str)
    plt.plot(mv.soc, mv.vb, color='cyan', linestyle='--', label='vb'+test_str)
    # plt.plot(so.soc_s, so.vb_s, color='black', linestyle='-.', label='vb_s'+ref_str)
    plt.plot(smv.soc_s, smv.vb_s, color='magenta', linestyle=':', label='vb_s'+test_str)
    plt.plot(mo.soc, mo.voc_stat, color='orange', linestyle='-', label='voc_stat'+ref_str)
    plt.xlabel('state-of-charge')
    plt.legend(loc=2)
    plt.subplot(337)
    plt.plot(mo.time, mo.vb, color='blue', linestyle='-', label='vb'+ref_str)
    plt.plot(mv.time, mv.vb, color='cyan', linestyle='--', label='vb'+test_str)
    # plt.plot(so.time, so.vb_s, color='black', linestyle='-.', label='vb_s'+ref_str)
    plt.plot(smv.time, smv.vb_s, color='magenta', linestyle=':', label='vb_s'+test_str)
    plt.xlabel('sec')
    plt.legend(loc=2)
    plt.subplot(338)
    plt.plot(mo.time, mo.dv_hys, color='blue', linestyle='-', label='dv_hys'+ref_str)
    plt.plot(mv.time, mv.dv_hys, color='cyan', linestyle='--', label='dv_hys'+test_str)
    # plt.plot(so.time, so.dv_hys_s, color='black', linestyle='-.', label='dv_hys_s'+ref_str)
    plt.plot(smv.time, smv.dv_hys_s, color='magenta', linestyle=':', label='dv_hys_s'+test_str)
    plt.xlabel('sec')
    plt.legend(loc=3)
    plt.subplot(339)
    plt.plot(mo.time, mo.Tb, color='blue', linestyle='-', label='Tb'+ref_str)
    plt.plot(mv.time, mv.tau_hys, color='cyan', linestyle='--', label='tau_hys' + test_str)
    plt.legend(loc=3)
    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return fig_list, fig_files


def calc_fault(d_ra, d_mod):
    falw = d_ra.falw.astype(int)
    fltw = d_ra.fltw.astype(int)
    dscn_fa = np.bool_(falw & 2 ** 10)
    ib_diff_fa = np.bool_((falw & 2 ** 8) | (falw & 2 ** 9))
    wv_fa = np.bool_(falw & 2 ** 7)
    wl_fa = np.bool_(falw & 2 ** 6)
    wh_fa = np.bool_(falw & 2 ** 5)
    ccd_fa = np.bool_(falw & 2 ** 4)
    ib_noa_fa = np.bool_(falw & 2 ** 3)
    ib_amp_fa = np.bool_(falw & 2 ** 2)
    vb_fa = np.bool_(falw & 2 ** 1)
    tb_fa = np.bool_(falw & 2 ** 0)
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
        ib_diff_flt = np.bool_(fltw & 2 ** 8) | (fltw & 2 ** 9)
        wh_flt = np.bool_(fltw & 2 ** 5)
        wl_flt = np.bool_(fltw & 2 ** 6)
        red_loss = np.bool_(fltw & 2 ** 7)
        dscn_flt = np.bool_(fltw & 2 ** 10)
        vb_flt = np.bool_(fltw & 2 ** 1)
        tb_flt = np.bool_(fltw & 2 ** 0)
        d_mod = rf.rec_append_fields(d_mod, 'ib_diff_flt', np.array(ib_diff_flt, dtype=float))
        d_mod = rf.rec_append_fields(d_mod, 'wh_flt', np.array(wh_flt, dtype=float))
        d_mod = rf.rec_append_fields(d_mod, 'wl_flt', np.array(wl_flt, dtype=float))
        d_mod = rf.rec_append_fields(d_mod, 'red_loss', np.array(red_loss, dtype=float))
        d_mod = rf.rec_append_fields(d_mod, 'dscn_flt', np.array(dscn_flt, dtype=float))
        d_mod = rf.rec_append_fields(d_mod, 'vb_flt', np.array(vb_flt, dtype=float))
        d_mod = rf.rec_append_fields(d_mod, 'tb_flt', np.array(tb_flt, dtype=float))
    except IOError:
        pass

    return d_mod


# Fake stuff to get replicate to accept inputs and run
def bandaid(h):
    res = np.zeros(len(h.time_ux))
    res[0:10] = 1
    ib_sel = h['ib'].copy()
    vb_sel = h['vb'].copy()
    tb_sel = h['Tb'].copy()
    voc = h['voc'].copy()
    ib_in_s = h['ib'].copy()
    soc_s = h['soc'].copy()
    bms_off_s = h['bms_off'].copy()
    sat_s = h['sat'].copy()
    chm_s = h['chm_s'].copy()
    sel = np.zeros(len(h.time_ux))
    preserving = np.ones(len(h.time_ux))
    mon_old = rf.rec_append_fields(h, 'res', res)
    mon_old = rf.rec_append_fields(mon_old, 'ib_past', ib_in_s)
    if not hasattr(mon_old, 'ib_sel'):
        mon_old = rf.rec_append_fields(mon_old, 'ib_sel', ib_sel)
    mon_old = rf.rec_append_fields(mon_old, 'tb_sel', tb_sel)
    if not hasattr(mon_old, 'voc'):
        mon_old = rf.rec_append_fields(mon_old, 'voc', voc)
    mon_old = rf.rec_append_fields(mon_old, 'preserving', preserving)
    mon_old = rf.rec_append_fields(mon_old, 'vb_sel', vb_sel)
    mon_old = rf.rec_append_fields(mon_old, 'soc_s', soc_s)
    mon_old = rf.rec_append_fields(mon_old, 'sel', sel)
    mon_old = rf.rec_append_fields(mon_old, 'ewh_thr', sel)
    mon_old = rf.rec_append_fields(mon_old, 'ewl_thr', sel)
    mon_old = rf.rec_append_fields(mon_old, 'ccd_thr', sel)
    mon_old = rf.rec_append_fields(mon_old, 'voc_ekf', sel)
    mon_old = rf.rec_append_fields(mon_old, 'y_ekf', sel)
    sim_old = np.array(np.zeros(len(h.time), dtype=[('time', '<f8')])).view(np.recarray)
    sim_old.time = mon_old.time.copy()
    sim_old = rf.rec_append_fields(sim_old, 'chm_s', chm_s)
    sim_old = rf.rec_append_fields(sim_old, 'sat_s', sat_s)
    sim_old = rf.rec_append_fields(sim_old, 'ib_in_s', ib_in_s)
    sim_old = rf.rec_append_fields(sim_old, 'bms_off_s', bms_off_s)
    sim_old = rf.rec_append_fields(sim_old, 'dv_dyn_s', bms_off_s)
    sim_old = rf.rec_append_fields(sim_old, 'dv_hys_s', bms_off_s)
    sim_old = rf.rec_append_fields(sim_old, 'voc_stat_s', bms_off_s)
    return mon_old, sim_old


def calculate_capacity(q_cap_rated_scaled=None, dqdt=None, temp=None, t_rated=None):
    q_cap = q_cap_rated_scaled * (1. + dqdt * (temp - t_rated))
    return q_cap


# Make an array useful for analysis (around temp) and add some metrics
def filter_Tb(raw, temp_corr, mon, tb_band=5., rated_batt_cap=100.):
    h = raw[abs(raw.Tb - temp_corr) < tb_band]

    sat_ = np.copy(h.Tb)
    bms_off_ = np.copy(h.Tb)
    for i in range(len(h.Tb)):
        sat_[i] = is_sat(h.Tb[i], h.voc[i], h.soc[i], mon.chemistry.nom_vsat, mon.chemistry.dvoc_dt,
                         mon.chemistry.low_t)
        # h.bms_off[i] = (h.Tb[i] < low_t) or ((h.voc[i] < low_voc) and (h.ib[i] < IB_MIN_UP))
        bms_off_[i] = (h.Tb[i] < mon.chemistry.low_t) or ((h.voc_stat[i] < 10.5) and (h.ib[i] < Battery.IB_MIN_UP))

    # Correct for temp
    q_cap = calculate_capacity(q_cap_rated_scaled=rated_batt_cap * 3600., dqdt=mon.chemistry.dqdt, temp=h.Tb,
                               t_rated=mon.chemistry.rated_temp)
    dq = (h.soc - 1.) * q_cap
    dq -= mon.chemistry.dqdt * q_cap * (temp_corr - h.Tb)
    q_cap_r = calculate_capacity(q_cap_rated_scaled=rated_batt_cap * 3600., dqdt=mon.chemistry.dqdt, temp=temp_corr,
                                 t_rated=mon.chemistry.rated_temp)
    soc_r = 1. + dq / q_cap_r
    h = rf.rec_append_fields(h, 'soc_r', soc_r)
    h.voc_stat_r = h.voc_stat - (h.Tb - temp_corr) * mon.chemistry.dvoc_dt
    h.voc_stat_rescaled_r = h.voc_stat_rescaled - (h.Tb - temp_corr) * mon.chemistry.dvoc_dt

    # delineate charging and discharging
    voc_stat_r_chg = np.copy(h.voc_stat)
    voc_stat_r_dis = np.copy(h.voc_stat)
    voc_stat_rescaled_r_chg = np.copy(h.voc_stat_rescaled)
    voc_stat_rescaled_r_dis = np.copy(h.voc_stat_rescaled)
    for i in range(len(voc_stat_r_chg)):
        if h.ib[i] > -0.5:
            voc_stat_r_dis[i] = None
            voc_stat_rescaled_r_dis[i] = None
        elif h.ib[i] < 0.5:
            voc_stat_r_chg[i] = None
            voc_stat_rescaled_r_chg[i] = None

    # Hysteresis_20220917d confirm equals data with HYS_SCALE_20220917d
    if len(h.time_ux) > 1:
        hys_remodel = Hysteresis_20220917d(scale=HYS_SCALE_20220917d)  # Battery hysteresis model - drift of voc
        t_s_min = h.time_min[0]
        t_e_min = h.time_min[-1]
        dt_hys_min = 1.
        dt_hys_sec = dt_hys_min * 60.
        hys_time_min = np.arange(t_s_min, t_e_min, dt_hys_min, dtype=float)
        # Note:  Hysteresis_20220917d instantiates hysteresis state to 0. unless told otherwise
        dv_hys_remodel = []
        for i in range(len(hys_time_min)):
            t_sec = hys_time_min[i] * 60.
            ib = np.interp(t_sec, h.time_sec, h.ib)
            soc = np.interp(t_sec, h.time_sec, h.soc)
            hys_remodel.calculate_hys(ib, soc)
            dvh = hys_remodel.update(dt_hys_sec)
            dv_hys_remodel.append(dvh)
        dv_hys_remodel = np.array(dv_hys_remodel)
        dv_hys_remodel_ = np.copy(h.soc)
        for i in range(len(h.time_ux)):
            t_min = int(float(h.time_ux[i]) / 60.)
            dv_hys_remodel_[i] = np.interp(t_min, hys_time_min, dv_hys_remodel)

        hys_redesign = Hysteresis_20220926(scale=HYS_SCALE_20220926, cap=HYS_CAP_REDESIGN)
        # Battery hysteresis model - drift of voc
        t_s_min = h.time_min[0]
        t_e_min = h.time_min[-1]
        dt_hys_min = 1.
        dt_hys_sec = dt_hys_min * 60.
        hys_time_min = np.arange(t_s_min, t_e_min, dt_hys_min, dtype=float)
        # Note:  Hysteresis_20220926 instantiates hysteresis state to 0. unless told otherwise
        dv_hys_redesign = []
        res_redesign = []
        ioc_redesign = []
        dv_dot_redesign = []
        voc_stat_redesign = []
        voc_stat_redesign_r = []
        for i in range(len(hys_time_min)):
            t_sec = hys_time_min[i] * 60
            tb = np.interp(t_sec, h.time_sec, h.Tb)
            ib = np.interp(t_sec, h.time_sec, h.ib)
            soc = np.interp(t_sec, h.time_sec, h.soc)
            soc_min = np.interp(t_sec, h.time_sec, h.soc_min)
            sat = np.interp(t_sec, h.time_sec, sat_)
            bms_off = np.interp(t_sec, h.time_sec, bms_off_) > 0.5
            voc = np.interp(t_sec, h.time_sec, h.voc)
            e_wrap = np.interp(t_sec, h.time_sec, h.e_wrap)
            hys_redesign.calculate_hys(ib, soc)
            init_low = bms_off or (soc < (soc_min + HYS_SOC_MIN_MARG) and ib > Battery.HYS_IB_THR)
            dvh = hys_redesign.update(dt_hys_sec, init_high=sat, init_low=init_low, e_wrap=e_wrap)
            res = hys_redesign.res
            ioc = hys_redesign.ioc
            dv_dot = hys_redesign.dv_dot
            voc_stat = voc - dvh
            voc_stat_r = voc_stat - (tb - temp_corr) * mon.chemistry.dvoc_dt
            dv_hys_redesign.append(dvh)
            res_redesign.append(res)
            ioc_redesign.append(max(min(ioc, 40.), -40.))
            dv_dot_redesign.append(dv_dot)
            voc_stat_redesign.append(voc_stat)
            voc_stat_redesign_r.append(voc_stat_r)
        dv_hys_redesign_ = np.copy(h.soc)
        res_redesign_ = np.copy(h.soc)
        ioc_redesign_ = np.copy(h.soc)
        dv_dot_redesign_ = np.copy(h.soc)
        voc_stat_redesign_ = np.copy(h.soc)
        voc_stat_redesign_r_ = np.copy(h.soc)
        for i in range(len(h.time_ux)):
            t_min = h.time_min[i]
            dv_hys_redesign_[i] = np.interp(t_min, hys_time_min, dv_hys_redesign)
            res_redesign_[i] = np.interp(t_min, hys_time_min, res_redesign)
            ioc_redesign_[i] = np.interp(t_min, hys_time_min, ioc_redesign)
            dv_dot_redesign_[i] = np.interp(t_min, hys_time_min, dv_dot_redesign)
            voc_stat_redesign_[i] = np.interp(t_min, hys_time_min, voc_stat_redesign)
            voc_stat_redesign_r_[i] = np.interp(t_min, hys_time_min, voc_stat_redesign_r)
        voc_stat_redesign_r_chg = np.copy(voc_stat_redesign_r_)
        voc_stat_redesign_r_dis = np.copy(voc_stat_redesign_r_)
        dv_hys_redesign_chg = np.copy(dv_hys_redesign_)
        dv_hys_redesign_dis = np.copy(dv_hys_redesign_)
        res_redesign_chg = np.copy(res_redesign_)
        res_redesign_dis = np.copy(res_redesign_)
        for i in range(len(voc_stat_r_chg)):
            if h.ib[i] > -0.5:
                voc_stat_redesign_r_dis[i] = None
                dv_hys_redesign_dis[i] = None
                res_redesign_dis[i] = None
            elif h.ib[i] < 0.5:
                voc_stat_redesign_r_chg[i] = None
                dv_hys_redesign_chg[i] = None
                res_redesign_chg[i] = None
        h = rf.rec_append_fields(h, 'voc_stat_redesign_r_dis', voc_stat_redesign_r_dis)
        h = rf.rec_append_fields(h, 'voc_stat_redesign_r_chg', voc_stat_redesign_r_chg)
        h = rf.rec_append_fields(h, 'dv_hys_redesign_dis', dv_hys_redesign_dis)
        h = rf.rec_append_fields(h, 'dv_hys_redesign_chg', dv_hys_redesign_chg)
        h = rf.rec_append_fields(h, 'res_redesign_dis', res_redesign_dis)
        h = rf.rec_append_fields(h, 'res_redesign_chg', res_redesign_chg)
        h = rf.rec_append_fields(h, 'sat', sat_)
        h = rf.rec_append_fields(h, 'bms_off', bms_off_)
        h = rf.rec_append_fields(h, 'dv_hys_remodel', dv_hys_remodel_)
        h = rf.rec_append_fields(h, 'voc_stat_r_dis', voc_stat_r_dis)
        h = rf.rec_append_fields(h, 'voc_stat_r_chg', voc_stat_r_chg)
        h = rf.rec_append_fields(h, 'voc_stat_rescaled_r_dis', voc_stat_rescaled_r_dis)
        h = rf.rec_append_fields(h, 'voc_stat_rescaled_r_chg', voc_stat_rescaled_r_chg)
        h = rf.rec_append_fields(h, 'ioc_redesign', ioc_redesign_)
        h = rf.rec_append_fields(h, 'dv_dot_redesign', dv_dot_redesign_)
        h = rf.rec_append_fields(h, 'dv_hys_redesign', dv_hys_redesign_)
        h = rf.rec_append_fields(h, 'res_redesign', res_redesign_)
        h = rf.rec_append_fields(h, 'voc_stat_redesign', voc_stat_redesign_)
        h = rf.rec_append_fields(h, 'voc_stat_redesign_r', voc_stat_redesign_r_)

    return h


# Table lookup vector
def look_it(x, tab, temp):
    voc = x.copy()
    for i in range(len(x)):
        voc[i] = tab.interp(x[i], temp)
    return voc


def shift_time(mo, extra_shift=0.):
    # Shift time
    first_non_zero = 0
    n = len(mo.time)
    while abs(mo.ib[first_non_zero]) < 0.02 and first_non_zero < n:
        first_non_zero += 1
    if first_non_zero < n:  # success
        if first_non_zero > 0:
            shift = (mo.time[first_non_zero] + mo.time[first_non_zero-1]) / 2.
        else:
            shift = mo.time[first_non_zero]
        print('shift time by', shift)
        mo.time = mo.time - shift + extra_shift
    return mo


def add_chm(hist, mon_t_=False, mon=None, chm=None):
    if mon_t_ is False or mon is None:
        print("add_chm:  not executing")
        if chm is not None:
            chm_s = []
            for i in range(len(hist.time)):
                chm_s.append(chm)
            hist = rf.rec_append_fields(hist, 'chm_s', np.array(chm_s, dtype=int))
            hist = rf.rec_append_fields(hist, 'chm', np.array(chm_s, dtype=int))
        return hist
    else:
        chm = []
        for i in range(len(hist.time_ux)):
            t_sec = float(hist.time_ux[i] - hist.time_ux[0]) + mon.time[0]
            chm.append(np.interp(t_sec, mon.time, mon.chm))
        hist = rf.rec_append_fields(hist, 'chm_s', np.array(chm, dtype=int))
        hist = rf.rec_append_fields(hist, 'chm', np.array(chm, dtype=int))
    return hist


def add_mod(hist, mon_t_=False, mon=None):
    if mon_t_ is False or mon is None:
        print("add_mod:  not executing")
        return hist
    else:
        mod_data = []
        for i in range(len(hist.time_ux)):
            t_sec = float(hist.time_ux[i]) - float(hist.time_ux[0]) + mon.time[0]
            mod_data.append(np.interp(t_sec, mon.time, mon.mod_data))
        return rf.rec_append_fields(hist, 'mod_data', np.array(mod_data, dtype=int))


def add_qcrs(hist, mon_t_=False, mon=None, qcrs=None):
    if mon_t_ is False or mon is None:
        print("add_qcrs:  not executing")
        if qcrs is not None:
            qcrs_m = []
            for i in range(len(hist.time)):
                qcrs_m.append(qcrs)
            hist = rf.rec_append_fields(hist, 'qcrs_s', np.array(qcrs_m, dtype=float))
            hist = rf.rec_append_fields(hist, 'qcrs', np.array(qcrs_m, dtype=float))
        return hist
    else:
        qcrs_m = []
        for i in range(len(hist.time_ux)):
            t_sec = float(hist.time_ux[i] - hist.time_ux[0]) + mon.time[0]
            qcrs_m.append(np.interp(t_sec, mon.time, mon.chm))
        hist = rf.rec_append_fields(hist, 'chm_s', np.array(qcrs_m, dtype=float))
        hist = rf.rec_append_fields(hist, 'chm', np.array(qcrs, dtype=float))
    return hist


def compare_hist_sim(data_file=None, time_end_in=None, rel_path_to_save_pdf='./figures', rel_path_to_temp='./temp',
                     data_only=False, mon_t=False, unit_key=None, sync_time=None, dt_resample=10, Tb_force=None):

    print(f"\ncompare_hist_sim:\n{data_file=}\n{rel_path_to_save_pdf=}\n{rel_path_to_temp=}\n{data_only=}\n{mon_t=}"
          f"\n{unit_key=}\n{dt_resample=}\n")

    date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
    date_ = datetime.now().strftime("%y%m%d")

    # Save these
    scale_in = 1
    cc_dif_tol_in = 0.2
    use_mon_soc_in = True
    rated_batt_cap_in = 100.
    # Reconstruction of soc using sub-sampled data is poor.  Drive everything with soc from Monitor
    dvoc_mon_in = 0.
    dvoc_sim_in = 0.

    # File path operations
    (data_file_folder, data_file_txt) = os.path.split(data_file)
    save_pdf_path = os.path.join(data_file_folder, rel_path_to_save_pdf)
    if not os.path.isdir(save_pdf_path):
        os.mkdir(save_pdf_path)
    path_to_temp = os.path.join(data_file_folder, rel_path_to_temp)
    if not os.path.isdir(path_to_temp):
        os.mkdir(path_to_temp)

    # Load mon to extract mod information
    # # Load mon v4 (old)
    if mon_t is True:
        mon_old, sim_old, f, mon_t_file_clean, temp_mont_t_file_clean = \
            load_data(data_file, 1, unit_key=unit_key, time_end_in=time_end_in, zero_zero_in=False)
    else:
        mon_old = None

    # Load history
    temp_hist_file_clean = write_clean_file(data_file, type_='_hist', hdr_key='fltb', unit_key='unit_f',
                                            skip=1, comment_str='---')
    if temp_hist_file_clean:
        h_raw = np.genfromtxt(temp_hist_file_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
    else:
        print("data from", temp_hist_file_clean, "empty after loading")
        tkinter.messagebox.showwarning(message="CompareHistSim:  Data missing.  See monitor window for info.")
        return None, None, None, None, None

    # Load fault
    temp_flt_file_clean = write_clean_file(data_file, type_='_flt', hdr_key='fltb', unit_key='unit_f',
                                           skip=1, comment_str='---')
    if temp_flt_file_clean:
        f_raw = np.genfromtxt(temp_flt_file_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
    else:
        print("data from", temp_flt_file_clean, "empty after loading")
        tkinter.messagebox.showwarning(message="CompareHistSim:  Data missing.  See monitor window for info.")
        return None, None, None, None, None

    # Load configuration
    unit = None
    if mon_t is True:
        chm = int(mon_old.chm[0])
    else:
        if unit_key.__contains__('bb'):
            chm = 0
        elif unit_key.__contains__('ch'):
            chm = 1
        else:
            chm = None
        unit = unit_key.split('_')[-2]
        if unit == 'pro3p2' or unit == 'soc2p2':
            rated_batt_cap_in = 102.9  # A-hr capacity of test article (output of Calibrate_exe.py)
        else:
            rated_batt_cap_in = 108.4  # A-hr capacity of test article
    qcrs = rated_batt_cap_in * Battery.UNIT_CAP_RATED * 3600.
    batt = BatteryMonitor(mod_code=chm, unit=unit)

    # Force Tb.  This is useful for verifying calibration runs where voc(soc) schedule extracted from the run
    # with slightly varying Tb but assumed constant when making new schedule
    if Tb_force is not None:
        f_raw.Tb = Tb_force
        h_raw.Tb = Tb_force

    # Sort and augment data
    f_raw = np.unique(f_raw)
    f_raw = remove_nan(f_raw)
    # noinspection PyTypeChecker
    f = add_stuff_f(f_raw, batt, ib_band=IB_BAND, rated_batt_cap=rated_batt_cap_in, Dw=dvoc_mon_in, time_sync=sync_time,
                    unit=unit)
    print("\nf:\n", f.dtype.names, f, "\n")

    f = filter_Tb(f, 20., batt, tb_band=100., rated_batt_cap=rated_batt_cap_in)
    h_raw = np.unique(h_raw)
    h_raw = remove_nan(h_raw)
    print("\nh raw:\n", h_raw.dtype.names, "\n", h_raw, "\n", h_raw.dtype.names, "\n")
    # noinspection PyTypeChecker
    h = add_stuff_f(h_raw, batt, ib_band=IB_BAND, rated_batt_cap=rated_batt_cap_in, Dw=dvoc_mon_in, time_sync=sync_time)
    h = add_mod(h, mon_t, mon_old)
    h = add_chm(h, mon_t, mon_old, chm)
    h = add_qcrs(h, mon_t_=mon_t, mon=mon_old, qcrs=qcrs)
    print("\nh after add_stuff:\n", h.dtype.names, "\n", h, "\n", h.dtype.names, "\n :h after add_stuff\n")

    # Convert all the long time readings (history) to same arbitrary (20 deg C) temperature
    h_20C = filter_Tb(h, 20., batt, tb_band=TB_BAND, rated_batt_cap=rated_batt_cap_in)

    # Shift time by detecting when ib changes
    if sync_time is None:
        h_20C = shift_time(h_20C)

    # Covert to fast update rate
    h_20C_resamp = resample(data=h_20C, dt_resamp=dt_resample, time_var='time',
                            specials=[('falw', 0), ('dscn_fa', 0), ('ib_diff_fa', 0), ('wv_fa', 0),
                                      ('wl_fa', 0), ('wh_fa', 0), ('ccd_fa', 0), ('ib_noa_fa', 0),
                                      ('ib_amp_fa', 0), ('vb_fa', 0), ('tb_fa', 0)])
    for i in range(len(h_20C_resamp.time)):
        if i == 0:
            h_20C_resamp.dt[i] = h_20C_resamp.time[1] - h_20C_resamp.time[0]
        else:
            h_20C_resamp.dt[i] = h_20C_resamp.time[i] - h_20C_resamp.time[i-1]
            
    # Hand fix oddities
    mon_old, sim_old = bandaid(h_20C_resamp)

    # Replicate
    data_file_clean = path_to_temp+'/'+data_file_txt.replace('.csv', '_hist' + '.csv', 1)
    mon_file_save = data_file_clean.replace(".csv", "_rep_hist.csv")
    mon_ver, sim_ver, sim_s_ver, mon_r, sim_r =\
        replicate(mon_old, sim_old=sim_old, init_time=1., verbose=False, t_max=time_end_in, use_vb_sim=False,
                  scale_in=scale_in, use_mon_soc=use_mon_soc_in, dvoc_mon=dvoc_mon_in, dvoc_sim=dvoc_sim_in,
                  unit=unit)
    save_clean_file(mon_ver, mon_file_save, 'mon_rep_hist' + date_)

    # Plots
    if data_only is False:
        fig_list = []
        fig_files = []
        data_root = temp_hist_file_clean.split('/')[-1].replace('.csv', '_')
        filename = data_root + sys.argv[0].split('/')[-1].split('\\')[-1].split('.')[-2]
        plot_title = filename + '   ' + date_time
        if len(f.time) > 1:
            fig_list, fig_files = over_fault(f, filename, fig_files=fig_files, plot_title=plot_title, subtitle='faults',
                                             fig_list=fig_list, cc_dif_tol=cc_dif_tol_in)
        if len(h_20C.time) > 1:
            sim_old = None
            plot_init_in = False
            fig_list, fig_files = dom_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename,
                                           fig_files, plot_title=plot_title, fig_list=fig_list,
                                           plot_init_in=plot_init_in, ref_str='', test_str='_ver')
            fig_list, fig_files = gp_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename,
                                          fig_files, plot_title=plot_title, fig_list=fig_list,
                                          ref_str='', test_str='_ver')
            fig_list, fig_files = off_on_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename,
                                              fig_files, plot_title=plot_title, fig_list=fig_list,
                                              ref_str='', test_str='_ver')
            fig_list, fig_files = overall_fault(mon_old, mon_ver, sim_ver, sim_s_ver, filename,
                                                fig_files, plot_title=plot_title, fig_list=fig_list)
            fig_list, fig_files = tune_r(mon_old, mon_ver, sim_s_ver, filename,
                                         fig_files, plot_title=plot_title, fig_list=fig_list,
                                         ref_str='', test_str='_ver')

        precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=save_pdf_path)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', save_pdf_path=save_pdf_path)
        cleanup_fig_files(fig_files)
    
        plt.show(block=False)
        string = 'plots ' + str(fig_list[0].number) + ' - ' + str(fig_list[-1].number)
        show_killer(string, 'CompareFault', fig_list=fig_list)

    return mon_old, sim_old, mon_ver, sim_ver, sim_s_ver


def main():
    # User inputs (multiple input_files allowed
    data_file = 'G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/hist 35C pro3p2_ch.csv'
    rel_path_to_save_pdf = 'G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/./figures'
    rel_path_to_temp = 'G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/./temp'
    data_only = False
    # mon_t = True
    mon_t = False
    unit_key = 'g20240331_pro3p2_ch'
    dt_resample = 10

    # Do this when running compare_hist_sim on run that schedule extracted assuming constant Tb
    tb_force = 35

    # cat(temp_hist_file, input_files, in_path=path_to_data, out_path=path_to_temp)

    compare_hist_sim(data_file=data_file, mon_t=mon_t, unit_key=unit_key, dt_resample=dt_resample,
                     rel_path_to_save_pdf=rel_path_to_save_pdf, rel_path_to_temp=rel_path_to_temp, data_only=data_only,
                     Tb_force=tb_force)


if __name__ == '__main__':
    main()
