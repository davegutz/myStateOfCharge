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

""" Python model of what's installed on the Particle Photon.  Includes
a monitor object (MON) and a simulation object (SIM).   The monitor is
the EKF and Coulomb Counter.   The SIM is a battery model, that also has a
Coulomb Counter built in."""
import sys

from MonSim import replicate, save_clean_file
from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
from CompareFault import over_fault
import matplotlib.pyplot as plt
from datetime import datetime
from load_data import load_data
from DataOverModel import dom_plot
from PlotSimS import sim_s_plot
from PlotEKF import ekf_plot
from PlotGP import tune_r, gp_plot
from PlotOffOn import off_on_plot
import easygui
from PlotKiller import show_killer
import tkinter.messagebox
from local_paths import version_from_data_file, local_paths
import os

if sys.platform == 'darwin':
    import matplotlib
    matplotlib.use('tkagg')
if sys.platform == 'darwin':
    import matplotlib
    matplotlib.use('tkagg')
plt.rcParams['axes.grid'] = True


def compare_run_sim(data_file=None, unit_key=None, time_end_in=None, data_only=False):
    print(f"\ncompare_run_sim:\n{data_file=}\n{unit_key=}\n{time_end_in=}\n{data_only=}\n")

    date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
    date_ = datetime.now().strftime("%y%m%d")
    
    # Transient  inputs
    zero_zero_in = False
    init_time_in = None
    use_vb_sim_in = False
    use_ib_mon_in = False
    tune_in = False
    cc_dif_tol_in = 0.2
    legacy_in = False
    ds_voc_soc_in = 0.
    use_vb_raw = False
    dvoc_sim_in = 0.
    dvoc_mon_in = 0.
    use_mon_soc_in = True

    # detect running interactively
    # this is written to run in pwd of call
    if data_file is None:
        path_to_data = easygui.fileopenbox(msg="choose your data file to plot")
        data_file = easygui.filesavebox(msg="pick new file name, cancel to keep", title="get new file name")
        if data_file is None:
            data_file = path_to_data
        else:
            os.rename(path_to_data, data_file)
        unit_key = easygui.enterbox(msg="enter pro0p, pro1a, soc0p, soc1a", title="get unit_key", default="pro1a")

    # Folder operations
    version = version_from_data_file(data_file)
    _, save_pdf_path, _ = local_paths(version)

    # # Load mon v4 (old)
    mon_old, sim_old, f, data_file_clean, temp_flt_file_clean, _ = \
        load_data(data_file, 1, unit_key, zero_zero_in, time_end_in, legacy=legacy_in)

    # How to initialize
    if mon_old is not None:
        if mon_old.time[0] == 0.:  # no initialization flat detected at beginning of recording
            init_time = 1.
        else:
            if init_time_in:
                init_time = init_time_in
            else:
                init_time = -4.
    else:
        tkinter.messagebox.showwarning(message="CompareRunSim:  Data missing.  See monitor window for info.")
        return None, None, None, None, None, None

    # New run
    mon_file_save = data_file_clean.replace(".csv", "_rep.csv")
    mon_ver, sim_ver, sim_s_ver, mon, sim = \
        replicate(mon_old, sim_old=sim_old, init_time=init_time, use_ib_mon=use_ib_mon_in, use_mon_soc=use_mon_soc_in,
                  use_vb_raw=use_vb_raw, dvoc_sim=dvoc_sim_in, dvoc_mon=dvoc_mon_in, use_vb_sim=use_vb_sim_in,
                  ds_voc_soc=ds_voc_soc_in)
    save_clean_file(mon_ver, mon_file_save, 'mon_rep' + date_)

    # Plots
    if data_only is False:
        fig_list = []
        fig_files = []
        dir_root_test, data_root_test = os.path.split(data_file_clean)
        data_root_test = data_root_test.replace('.csv', '')
        filename = data_root_test
        plot_title = dir_root_test + '/' + data_root_test + '   ' + date_time
        if temp_flt_file_clean and len(f.time_ux) > 1:
            fig_list, fig_files = over_fault(f, filename, fig_files=fig_files, plot_title=plot_title, subtitle='faults',
                                             fig_list=fig_list, cc_dif_tol=cc_dif_tol_in)
        fig_list, fig_files = dom_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                       plot_title=plot_title, fig_list=fig_list, ref_str='',
                                       test_str='_ver')
        fig_list, fig_files = ekf_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                       plot_title=plot_title, fig_list=fig_list, ref_str='',
                                       test_str='_ver')
        fig_list, fig_files = sim_s_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                         plot_title=plot_title, fig_list=fig_list, ref_str='',
                                         test_str='_ver')
        fig_list, fig_files = gp_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                      plot_title=plot_title, fig_list=fig_list, ref_str='',
                                      test_str='_ver')
        fig_list, fig_files = off_on_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                          plot_title=plot_title, fig_list=fig_list, ref_str='',
                                          test_str='_ver')
        if tune_in:
            fig_list, fig_files = tune_r(mon_old, mon_ver, sim_s_ver, filename, fig_files,
                                         plot_title=plot_title, fig_list=fig_list, ref_str='', test_str='_ver')

        # Copies
        precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=save_pdf_path)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', save_pdf_path=save_pdf_path)
        cleanup_fig_files(fig_files)
        plt.show(block=False)
        string = 'plots ' + str(fig_list[0].number) + ' - ' + str(fig_list[-1].number)
        show_killer(string, 'CompareRunSim', fig_list=fig_list)

    return data_file_clean, mon_old, sim_old, mon_ver, sim_ver, sim_s_ver


def main():
    data_file = 'G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/discharge_ib_soc_soc3p2_chg.csv'
    unit_key = 'g20240331_soc3p2_chg'
    time_end_in = None
    data_only = False

    compare_run_sim(data_file=data_file, unit_key=unit_key, data_only=data_only, time_end_in=time_end_in)


# import cProfile
# if __name__ == '__main__':
#     cProfile.run('main()')
#


if __name__ == '__main__':
    main()
