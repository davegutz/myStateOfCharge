# CompareRunHist.py:  combine a CompareRunSim with CompareHistSim
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

""" Python model of what's installed on the Particle Photon.  Includes
a monitor object (MON) and a simulation object (SIM).   The monitor is
the EKF and Coulomb Counter.   The SIM is a battery model, that also has a
Coulomb Counter built in."""

from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
import matplotlib.pyplot as plt
from DataOverModel import dom_plot
from PlotKiller import show_killer
from CompareRunSim import compare_run_sim
from CompareHistSim import compare_hist_sim
from datetime import datetime
import os
import tkinter.messagebox
from local_paths import version_from_data_file, local_paths

plt.rcParams['axes.grid'] = True


def compare_run_hist(data_file_=None, unit_key_=None, time_end_in_=None, data_only_=True):

    print(f"\ncompare_run_hist:\n{data_file_=}\n{data_only_=}\n{unit_key_=}\n{time_end_in_=}")

    date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
    # date_ = datetime.now().strftime("%y%m%d")

    dfcs, mo_r, so_r, mv_r, sv_r, ssv_r =\
        compare_run_sim(data_file=data_file_, unit_key=unit_key_, time_end_in=time_end_in_, data_only=data_only_)
    mo_h, so_h, mv_h, sv_h, ssv_h =\
        compare_hist_sim(data_file=data_file_, unit_key=unit_key_, time_end_in=time_end_in_, data_only=data_only_,
                         mon_t=True, sync_time=mo_r.time_ref)

    # Plots
    if mo_r is not None and mo_h is not None:
        fig_list = []
        fig_files = []

        # File path operations
        version = version_from_data_file(data_file_)
        _, save_pdf_path, _ = local_paths(version)

        (data_file_folder, _) = os.path.split(data_file_)

        data_root_ref = dfcs.split('/')[-1].replace('.csv', '')
        dir_root_ref = data_file_folder.split('/')[-1].split('\\')[-1]
        filename = data_root_ref + '__hist'

        # Plots
        plot_title = dir_root_ref + '/' + data_root_ref + '   ' + date_time

        fig_list, fig_files = dom_plot(mo_r, mo_h, so_r, so_h, ssv_h, filename, fig_files,
                                       plot_title=plot_title, fig_list=fig_list,
                                       ref_str='_run', test_str='_hist')  # all over all

        # Copies
        precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=save_pdf_path)
        unite_pictures_into_pdf(outputPdfName=filename+'-'+date_time+'.pdf', save_pdf_path=save_pdf_path,
                                listWithImagesExtensions=["png"])
        cleanup_fig_files(fig_files)
        plt.show(block=False)
        string = 'plots ' + str(fig_list[0].number) + ' - ' + str(fig_list[-1].number)
        show_killer(string, 'CompareRunRun', fig_list=fig_list)

        return True
    else:
        tkinter.messagebox.showwarning(message="One or more sets of data missing.  See monitor window for info.")
        return False


def main():
    data_file_full = \
        '/home/daveg/google-drive/GitHubArchive/SOC_Particle/dataReduction/g20231111b/rapidTweakRegression_pro1a_bb.csv'
    unit_key_full = 'g20231111b_pro1a_bb'

    compare_run_hist(data_file_=data_file_full, unit_key_=unit_key_full)


if __name__ == '__main__':
    main()
