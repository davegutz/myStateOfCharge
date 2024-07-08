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
import numpy as np

from DataOverModel import dom_plot
from CompareFault import over_fault
from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
import matplotlib.pyplot as plt
from datetime import datetime
from PlotKiller import show_killer
import os
from load_data import load_data, SyncInfo, calculate_master_sync
from local_paths import version_from_data_path, local_paths

import sys
if sys.platform == 'darwin':
    import matplotlib
    matplotlib.use('tkagg')
plt.rcParams['axes.grid'] = True


def compare_run_run(keys=None, data_file_folder_ref=None, data_file_folder_test=None, sync_to_ctime=False):

    print(f"compare_run_run:\n{keys=}\n{data_file_folder_ref=}\n{data_file_folder_test=}\n{sync_to_ctime=}\n")

    date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
    # date_ = datetime.now().strftime("%y%m%d")

    # Transient  inputs
    zero_zero_in = False
    time_end_in = None
    rated_batt_cap_ref_in = 108.4
    rated_batt_cap_test_in = 108.4

    # Regression suite
    data_file_txt_ref = keys[0][0]
    unit_key_ref = keys[0][1]
    data_file_txt_test = keys[1][0]
    unit_key_test = keys[1][1]

    # Folder operations
    version = version_from_data_path(data_file_folder_test)
    _, save_pdf_path, _ = local_paths(version)

    # Load old ref data
    data_file_ref = os.path.join(data_file_folder_ref, data_file_txt_ref)
    mon_ref, sim_ref, f_ref, data_file_ref_clean, temp_flt_file_ref_clean, sync_info_ref = \
        load_data(data_file_ref, 1, unit_key_ref, zero_zero_in, time_end_in,
                  rated_batt_cap=rated_batt_cap_ref_in)

    # Load new test data
    data_file_test = os.path.join(data_file_folder_test, data_file_txt_test)
    mon_test, sim_test, f_test, data_file_test_clean, temp_flt_file_test_clean, sync_info_test = \
        load_data(data_file_test, 1, unit_key_test, zero_zero_in, time_end_in,
                  rated_batt_cap=rated_batt_cap_test_in)

    # Synchronize
    # Time since beginning of data to sync pulses
    if sync_info_ref.is_empty is False and sync_info_test.is_empty is False and \
            sync_info_ref.length == sync_info_test.length and (sync_info_ref.length > 0 or sync_to_ctime is True):
        # Make target sync vector
        master_sync_del = calculate_master_sync(sync_info_ref.del_mon, sync_info_test.del_mon)
        sync_info_ref.synchronize(master_sync_del)
        mon_ref.time = sync_info_ref.time_mon.copy()
        sync_info_test.synchronize(master_sync_del)
        mon_test.time = sync_info_test.time_mon.copy()
        # print(f"{sync_to_ctime=}\n{sync_info_ref.del_mon=}\n{sync_info_test.del_mon=}\n{master_sync_del=}\n{mon_test.time=}")
    elif sync_to_ctime is True:
        cTime_0_ref = mon_ref.cTime[0]
        cTime_0_test = mon_test.cTime[0]
        cTime_sync = cTime_0_ref
        mon_ref.time = mon_ref.cTime - cTime_sync
        mon_test.time = mon_test.cTime - cTime_sync
    else:
        print(f"Using simplified sync with |Ib|>0.  Data sets too small to sync or not equivalent number of sync pulses")

    # Plots
    fig_list = []
    fig_files = []
    dir_root_ref, data_root_ref = os.path.split(data_file_ref_clean)
    data_root_ref = data_root_ref.replace('.csv', '')
    dir_root_test, data_root_test = os.path.split(data_file_test_clean)
    data_root_test = data_root_test.replace('.csv', '')

    filename = data_root_ref + '__' + data_root_test
    plot_title = dir_root_ref + '/' + data_root_ref + '__' + dir_root_test + '/' + data_root_test + '   ' + date_time

    if temp_flt_file_ref_clean and len(f_ref.time_ux) > 1:
        fig_list, fig_files = over_fault(f_ref, filename, fig_files=fig_files, plot_title=plot_title, subtitle='faults',
                                         fig_list=fig_list)

    fig_list, fig_files = dom_plot(mon_ref, mon_test, sim_ref, sim_test, sim_test, filename, fig_files,
                                   plot_title=plot_title, fig_list=fig_list)  # all over all

    # Copies
    precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=save_pdf_path)
    unite_pictures_into_pdf(outputPdfName=filename+'-'+date_time+'.pdf', save_pdf_path=save_pdf_path,
                            listWithImagesExtensions=["png"])
    cleanup_fig_files(fig_files)
    plt.show(block=False)
    string = 'plots ' + str(fig_list[0].number) + ' - ' + str(fig_list[-1].number)
    show_killer(string, 'CompareRunRun', fig_list=fig_list)

    return True


def main():
    keys = [('ampHiFail_pro1a_bb.csv', 'g20240331_pro1a_bb'), ('ampHiFail_pro1a_bb.csv', 'g20240704_pro1a_bb')]
    data_file_folder_ref = 'G:/My Drive/GitHubArchive/SOC_Particle/dataReduction\\g20240331'
    data_file_folder_test = 'G:/My Drive/GitHubArchive/SOC_Particle/dataReduction\\g20240704'
    sync_to_ctime = False
    compare_run_run(keys=keys, data_file_folder_ref=data_file_folder_ref, data_file_folder_test=data_file_folder_test,
                    sync_to_ctime=sync_to_ctime)


if __name__ == '__main__':
    main()
