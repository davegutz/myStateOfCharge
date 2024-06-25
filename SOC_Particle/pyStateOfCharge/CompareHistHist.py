# CompareHistSim.py:  load fault, hist, summ data and compare to simulation.
# Copyright (C) 2024 Dave Gutz
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
from PlotKiller import show_killer
from DataOverModel import dom_plot
from PlotGP import tune_r, gp_plot
from PlotOffOn import off_on_plot
from Chemistry_BMS import ib_lag
from myFilters import LagExp
from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
from datetime import datetime
from local_paths import version_from_data_file, local_paths
import os
from CompareHistSim import load_hist_and_prep, overall_fault, over_fault

import sys
if sys.platform == 'darwin':
    import matplotlib
    matplotlib.use('tkagg')
plt.rcParams['axes.grid'] = True

#  For this battery Battleborn 100 Ah with 1.084 x capacity
IB_BAND = 1.  # Threshold to declare charging or discharging
TB_BAND = 25.  # Band around temperature to group data and correct.  Large value means no banding, effectively
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
    while abs(mo.ib[first_non_zero]) < 0.02 and first_non_zero < n-1:
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
    return hist


def compare_hist_hist(data_file_ref=None, unit_key_ref=None, data_file_tst=None, unit_key_tst=None,
                      dt_resample=10, data_only=False):

    print(f"\ncompare_hist_sim:\n{data_file_ref=}\n{unit_key_ref=}\n{data_file_tst=}\n{unit_key_tst=}\n{dt_resample=}\n")

    date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")

    # Save these
    cc_dif_tol_in = 0.2
    sim_s_tst = None

    # Load history, normalizing all soc and Tb to 20C
    mon_ref, sim_ref, unit_ref, fault_ref, hist_20C_ref, filename_ref = \
        load_hist_and_prep(data_file=data_file_ref, unit_key=unit_key_ref, dt_resample=dt_resample)
    mon_tst, sim_tst, unit_tst, fault_tst, hist_20C_tst, filename_tst = \
        load_hist_and_prep(data_file=data_file_tst, unit_key=unit_key_tst, dt_resample=dt_resample)

    # File path operations
    _, data_file_txt = os.path.split(data_file_ref)
    version = version_from_data_file(data_file_ref)
    path_to_temp, save_pdf_path, _ = local_paths(version)

    # Plots
    if data_only is False:
        fig_list = []
        fig_files = []
        plot_title = filename_ref + filename_tst + '   ' + date_time
        if fault_ref is not None and len(fault_ref.time) > 1:
            fig_list, fig_files = over_fault(fault_ref, filename_ref, fig_files=fig_files, plot_title=plot_title,
                                             subtitle='faults_ref', fig_list=fig_list, cc_dif_tol=cc_dif_tol_in,
                                             time_units='sec')
        if fault_tst is not None and len(fault_tst.time) > 1:
            fig_list, fig_files = over_fault(fault_tst, filename_tst, fig_files=fig_files, plot_title=plot_title,
                                             subtitle='faults_tst', fig_list=fig_list, cc_dif_tol=cc_dif_tol_in,
                                             time_units='sec')
        if hist_20C_ref is not None and len(hist_20C_ref.time) > 1:
            sim_ref = None
            plot_init_in = False
            fig_list, fig_files = dom_plot(mon_ref, mon_tst, sim_ref, sim_tst, sim_s_tst, filename_ref,
                                           fig_files, plot_title=plot_title, fig_list=fig_list,
                                           plot_init_in=plot_init_in, ref_str='', test_str='_tst')
            fig_list, fig_files = gp_plot(mon_ref, mon_tst, sim_ref, sim_tst, sim_s_tst, filename_ref,
                                          fig_files, plot_title=plot_title, fig_list=fig_list,
                                          ref_str='', test_str='_tst')
            fig_list, fig_files = off_on_plot(mon_ref, mon_tst, sim_ref, sim_tst, sim_s_tst, filename_ref,
                                              fig_files, plot_title=plot_title, fig_list=fig_list,
                                              ref_str='', test_str='_tst')
            fig_list, fig_files = overall_fault(mon_ref, mon_tst, sim_tst, sim_s_tst, filename_ref,
                                                fig_files, plot_title=plot_title, fig_list=fig_list)
            fig_list, fig_files = tune_r(mon_ref, mon_tst, sim_s_tst, filename_ref,
                                         fig_files, plot_title=plot_title, fig_list=fig_list,
                                         ref_str='', test_str='_tst')

        precleanup_fig_files(output_pdf_name=filename_ref, path_to_pdfs=save_pdf_path)
        unite_pictures_into_pdf(outputPdfName=filename_ref+'_'+date_time+'.pdf', save_pdf_path=save_pdf_path)
        cleanup_fig_files(fig_files)
    
        plt.show(block=False)
        string = 'plots ' + str(fig_list[0].number) + ' - ' + str(fig_list[-1].number)
        show_killer(string, 'CompareFault', fig_list=fig_list)

    return mon_ref, sim_ref, mon_tst, sim_tst, sim_s_tst


def main():

    # User inputs (multiple input_files allowed
    data_file_ref = 'G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc0p hist 2024_06_25.csv'
    unit_key_ref = 'g20240331_soc0p_ch'
    data_file_tst = 'G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc3p2 hist 2024_06_25.csv'
    unit_key_tst = 'g20240331_soc3p2_ch'
    dt_resample = 10

    # Do this when running compare_hist_sim on run that schedule extracted assuming constant Tb
    # Tb_force = 35

    compare_hist_hist(data_file_ref=data_file_ref, unit_key_ref=unit_key_ref,
                      data_file_tst=data_file_tst, unit_key_tst=unit_key_tst,
                      dt_resample=dt_resample)


if __name__ == '__main__':
    main()
