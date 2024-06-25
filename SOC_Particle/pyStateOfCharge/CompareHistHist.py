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

import matplotlib.pyplot as plt
from PlotKiller import show_killer
from DataOverModel import dom_plot
from PlotGP import tune_r, gp_plot
from PlotOffOn import off_on_plot
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

    # Synchronize
    d_time = mon_tst.time_ux[0] - mon_ref.time_ux[0]
    if d_time > 0:
        mon_tst.time += d_time
    else:
        mon_ref.time -= d_time

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
                                           plot_init_in=plot_init_in, ref_str='_'+unit_ref, test_str='_'+unit_tst)
            fig_list, fig_files = gp_plot(mon_ref, mon_tst, sim_ref, sim_tst, sim_s_tst, filename_ref,
                                          fig_files, plot_title=plot_title, fig_list=fig_list,
                                          ref_str='_'+unit_ref, test_str='_'+unit_tst)
            fig_list, fig_files = off_on_plot(mon_ref, mon_tst, sim_ref, sim_tst, sim_s_tst, filename_ref,
                                              fig_files, plot_title=plot_title, fig_list=fig_list,
                                              ref_str='_'+unit_ref, test_str='_'+unit_tst)
            fig_list, fig_files = overall_fault(mon_ref, mon_tst, sim_tst, sim_s_tst, filename_ref,
                                                fig_files, plot_title=plot_title, fig_list=fig_list)
            fig_list, fig_files = tune_r(mon_ref, mon_tst, sim_s_tst, filename_ref,
                                         fig_files, plot_title=plot_title, fig_list=fig_list,
                                         ref_str='_'+unit_ref, test_str='_'+unit_tst)

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
