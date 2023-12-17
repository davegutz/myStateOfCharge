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
from DataOverModel import SavedData, SavedDataSim, write_clean_file, dom_plot
from CompareFault import add_stuff_f, over_fault, filter_Tb, IB_BAND
from Battery import Battery, BatteryMonitor
from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
import matplotlib.pyplot as plt
from datetime import datetime
from PlotKiller import show_killer

plt.rcParams['axes.grid'] = True


# Load from files
def load_data(path_to_data, skip, unit_key, zero_zero_in, time_end_in, rated_batt_cap=Battery.UNIT_CAP_RATED,
              legacy=False, v1_only=False):
    title_key = "unit,"  # Find one instance of title
    title_key_sel = "unit_s,"  # Find one instance of title
    unit_key_sel = "unit_sel"
    title_key_ekf = "unit_e,"  # Find one instance of title
    unit_key_ekf = "unit_ekf"
    title_key_sim = "unit_m,"  # Find one instance of title
    unit_key_sim = "unit_sim"
    temp_flt_file = 'flt_compareRunSim.txt'
    data_file_clean = write_clean_file(path_to_data, type_='_mon', title_key=title_key, unit_key=unit_key, skip=skip)
    if not legacy:
        cols = ('cTime', 'dt', 'chm', 'qcrs', 'sat', 'sel', 'mod', 'bmso', 'Tb', 'vb', 'ib', 'ib_charge', 'voc_soc',
                'vsat', 'dv_dyn', 'voc_stat', 'voc_ekf', 'y_ekf', 'soc_s', 'soc_ekf', 'soc')
    else:
        cols = ('cTime', 'dt', 'chm', 'sat', 'sel', 'mod', 'bmso', 'Tb', 'vb', 'ib', 'ib_charge', 'voc_soc',
                'vsat', 'dv_dyn', 'voc_stat', 'voc_ekf', 'y_ekf', 'soc_s', 'soc_ekf', 'soc')
    mon_raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols,  dtype=float,
                            encoding=None).view(np.recarray)

    # Load sel (old)
    sel_file_clean = write_clean_file(path_to_data, type_='_sel', title_key=title_key_sel,
                                      unit_key=unit_key_sel, skip=skip)
    cols_sel = ('c_time', 'res', 'user_sel', 'cc_dif',
                'ibmh', 'ibnh', 'ibmm', 'ibnm', 'ibm', 'ib_diff', 'ib_diff_f',
                'voc_soc', 'e_w', 'e_w_f',
                'ib_sel_stat', 'ib_h', 'ib_s', 'mib', 'ib',
                'vb_sel', 'vb_h', 'vb_s', 'mvb', 'vb',
                'Tb_h', 'Tb_s', 'mtb', 'Tb_f',
                'fltw', 'falw', 'ib_rate', 'ib_quiet', 'tb_sel',
                'ccd_thr', 'ewh_thr', 'ewl_thr', 'ibd_thr', 'ibq_thr', 'preserving')
    if sel_file_clean and not v1_only:
        sel_raw = np.genfromtxt(sel_file_clean, delimiter=',', names=True, usecols=cols_sel, dtype=float,
                                encoding=None).view(np.recarray)
    else:
        sel_raw = None

    # Load ekf (old)
    ekf_file_clean = write_clean_file(path_to_data, type_='_ekf', title_key=title_key_ekf,
                                      unit_key=unit_key_ekf, skip=skip)

    cols_ekf = ('c_time', 'Fx_', 'Bu_', 'Q_', 'R_', 'P_', 'S_', 'K_', 'u_', 'x_', 'y_', 'z_', 'x_prior_',
                'P_prior_', 'x_post_', 'P_post_', 'hx_', 'H_')
    ekf_raw = None
    if ekf_file_clean and not v1_only:
        ekf_raw = np.genfromtxt(ekf_file_clean, delimiter=',', names=True, usecols=cols_ekf, dtype=float,
                                encoding=None).view(np.recarray)

    mon = SavedData(data=mon_raw, sel=sel_raw, ekf=ekf_raw, time_end=time_end_in,
                    zero_zero=zero_zero_in)
    batt = BatteryMonitor(mon.chm[0])

    # Load sim _s v24 portion of real-time run (old)
    data_file_sim_clean = write_clean_file(path_to_data, type_='_sim', title_key=title_key_sim,
                                           unit_key=unit_key_sim, skip=skip)
    if not legacy:
        cols_sim = ('c_time', 'chm_s', 'qcrs_s', 'bmso_s', 'Tb_s', 'Tbl_s', 'vsat_s', 'voc_stat_s', 'dv_dyn_s', 'vb_s', 'ib_s',
                    'ib_in_s', 'ib_charge_s', 'ioc_s', 'sat_s', 'dq_s', 'soc_s', 'reset_s')
    else:
        cols_sim = ('c_time', 'chm_s', 'bmso_s', 'Tb_s', 'Tbl_s', 'vsat_s', 'voc_stat_s', 'dv_dyn_s', 'vb_s', 'ib_s',
                    'ib_in_s', 'ib_charge_s', 'ioc_s', 'sat_s', 'dq_s', 'soc_s', 'reset_s')
    if data_file_sim_clean and not v1_only:
        sim_raw = np.genfromtxt(data_file_sim_clean, delimiter=',', names=True, usecols=cols_sim,
                                dtype=float, encoding=None).view(np.recarray)
        sim = SavedDataSim(time_ref=mon.time_ref, data=sim_raw, time_end=time_end_in)
    else:
        sim = None

    # Load fault
    temp_flt_file_clean = write_clean_file(path_to_data, type_='_flt', title_key='fltb',
                                           unit_key='unit_f', skip=skip, comment_str='---')
    cols_f = ('time', 'Tb_h', 'vb_h', 'ibmh', 'ibnh', 'Tb', 'vb', 'ib', 'soc', 'soc_ekf', 'voc', 'voc_stat',
              'e_w_f', 'fltw', 'falw')
    if temp_flt_file_clean and not v1_only:
        f_raw = np.genfromtxt(temp_flt_file_clean, delimiter=',', names=True, usecols=cols_f, dtype=None,
                              encoding=None).view(np.recarray)
    else:
        print("data from", temp_flt_file, "empty after loading")
    if temp_flt_file_clean and not v1_only:
        f_raw = np.unique(f_raw)
        f = add_stuff_f(f_raw, batt, ib_band=IB_BAND)
        print("\nf:\n", f, "\n")
        f = filter_Tb(f, 20., batt, tb_band=100., rated_batt_cap=rated_batt_cap)
    else:
        f = None
    return mon, sim, f, data_file_clean, temp_flt_file_clean


def compare_run_run(keys=None, dir_data_ref_path=None, dir_data_test_path=None,
                    save_pdf_path='../dataReduction/figures', path_to_temp='../dataReduction/temp'):

    date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")

    # Transient  inputs
    skip = 1
    zero_zero_in = False
    time_end_in = None
    plot_init_in = False
    long_term_in = False
    plot_overall_in = True
    rated_batt_cap_in = 108.4
    legacy_in_ref = False
    legacy_in_test = False
    # path_to_data = '../dataReduction'
    import os
    if not os.path.isdir(save_pdf_path):
        os.mkdir(save_pdf_path)
    if not os.path.isdir(path_to_temp):
        os.mkdir(path_to_temp)

    # Regression
    # if keys is None:
        # keys = [('ampHiFail vA20221220.txt', 'soc1a_2022', 'legacy'), ('ampHiFail v20230128.txt', 'pro0p', 'legacy')]
        # keys = [('ampHiFail v20230128.txt', 'pro0p_20221220', 'legacy'), ('ampHiFail v20230303 CH.txt', 'pro0p_2023', 'legacy')]
        # keys = [('ampHiFail v20230303 CH.txt', 'pro0p_2023', 'legacy'), ('ampHiFail v20230305 CH.txt', 'pro0p_2023')];
        # keys = [('ampHiFail vA20221220.txt', 'soc1a_2022', 'legacy'), ('ampHiFail vA20230305 BB.txt', 'pro1a_2023')];
        # keys = [('rapidTweakRegression vA20230219.txt', 'pro1a_2023', 'legacy'), ('rapidTweakRegression vA20230305 BB.txt', 'pro1a_2023')];
        # keys = [('rapidTweakRegression v20230305 CH.txt', 'pro0p_2023'), ('rapidTweakRegression vA20230305 BB.txt', 'pro1a_2023')];
        # keys = [('rapidTweakRegression v20230305 CH.txt', 'pro0p'), ('rapidTweakRegression vA20230305 CH.txt', 'pro1a')]
        # keys = [('offSitHysBms vA20221220.txt', 'soc1a', 'legacy'), ('offSitHysBms vA20230305 BB.txt', 'pro1a_2023')];
        # keys = [('offSitHysBms v20230303 CH.txt', 'pro0p_2023', 'legacy'), ('offSitHysBms v20230305 CH.txt', 'pro0p_2023')];
        # keys = [('ampLoFail v20230303 CH.txt', 'pro0p', 'legacy'), ('ampLoFail v20230305 CH.txt', 'pro0p')]
        # Compare
        # keys = [('offSitHysBms v20221220.txt', 'pro0p_2022', 'legacy'), ('offSitHysBms vA20221220.txt', 'soc1a_2022', 'legacy')]
        # keys = [('ampHiFail v20230305 CH.txt', 'pro0p_2023'), ('ampHiFail vA20230305 BB.txt', 'pro1a_2023')];
        # keys = [('slowTweakRegression v20230305 CH.txt', 'pro0p_2023'), ('slowTweakRegression vA20230305 BB.txt', 'pro1a_2023')];

    # Regression suite
    data_file_ref_txt = keys[0][0]
    unit_key_ref = keys[0][1]
    if len(keys[0]) > 2 and keys[0][2] == 'legacy':
        legacy_in_ref = True
    data_file_test_txt = keys[1][0]
    unit_key_test = keys[1][1]
    if len(keys[1]) > 2 and keys[1][2] == 'legacy':
        legacy_in_test = True

    # Load old ref data
    data_file_ref_path = os.path.join(dir_data_ref_path, data_file_ref_txt)
    mon_ref, sim_ref, f_ref, data_file_ref_clean, temp_flt_file_ref_clean = \
        load_data(data_file_ref_path, skip, unit_key_ref, zero_zero_in, time_end_in,
                  rated_batt_cap=rated_batt_cap_in, legacy=legacy_in_ref)

    # Load new test data
    data_file_test_path = os.path.join(dir_data_test_path, data_file_test_txt)
    mon_test, sim_test, f_test, data_file_test_clean, temp_flt_file_test_clean = \
        load_data(data_file_test_path, skip, unit_key_test, zero_zero_in, time_end_in,
                  rated_batt_cap=rated_batt_cap_in, legacy=legacy_in_test)

    # Plots
    fig_list = []
    fig_files = []
    data_root_ref = data_file_ref_clean.split('/')[-1].replace('.csv', '')
    data_root_test = data_file_test_clean.split('/')[-1].replace('.csv', '')
    dir_root_ref = dir_data_ref_path.split('/')[-1].split('\\')[-1]
    dir_root_test = dir_data_test_path.split('/')[-1].split('\\')[-1]
    filename = data_root_ref + '__' + data_root_test
    plot_title = dir_root_ref + '/' + data_root_ref + '__' + dir_root_test + '/' + data_root_test + '   ' + date_time
    if temp_flt_file_ref_clean and len(f_ref.time) > 1:
        fig_list, fig_files = over_fault(f_ref, filename, fig_files=fig_files, plot_title=plot_title, subtitle='faults',
                                                   fig_list=fig_list, long_term=long_term_in)
    if plot_overall_in:
        fig_list, fig_files = dom_plot(mon_ref, mon_test, sim_ref, sim_test, sim_test, filename, fig_files,
                                       plot_title=plot_title, fig_list=fig_list, plot_init_in=plot_init_in)  # all over all
    precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=save_pdf_path)
    unite_pictures_into_pdf(outputPdfName=filename+'-'+date_time+'.pdf', save_pdf_path=save_pdf_path,
                            listWithImagesExtensions=["png"])
    cleanup_fig_files(fig_files)

    plt.show(block=False)
    string = 'plots ' + str(fig_list[0].number) + ' - ' + str(fig_list[-1].number)
    show_killer(string,'CompareRunRun', fig_list=fig_list)

    return True


if __name__ == '__main__':
    compare_run_run()
