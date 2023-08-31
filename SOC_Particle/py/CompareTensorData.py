# CompareTensorData:  Load long term hysteresis data and extract and save
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

""" Extract and save long term hysteresis data from file and save for regression by TrainTensor.py for example."""
import numpy as np

from MonSim import replicate, save_clean_file
from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
from CompareFault import over_fault
import matplotlib.pyplot as plt
from datetime import datetime
from CompareRunRun import load_data
from DataOverModel import dom_plot
from PlotSimS import sim_s_plot
from PlotEKF import ekf_plot
from PlotGP import tune_r, gp_plot
from PlotOffOn import off_on_plot
import os
from myFilters import LagExp
import numpy.lib.recfunctions as rf
from Chemistry_BMS import ib_lag, Chemistry
plt.rcParams['axes.grid'] = True


# Add ib_lag = ib lagged by time constant
def add_ib_lag(data):
    lag_tau = ib_lag(data.chm[0])
    IbLag = LagExp(1., lag_tau, -100., 100.)
    n = len(data.cTime)
    if data.ib_lag is None:
        data = rf.rec_append_fields(data, 'ib_lag', np.array(data.sat, dtype=float))
        data.ib_lag = np.zeros(n)
    dt = data.cTime[1] - data.cTime[0]
    for i in range(n):
        if i > 0:
            dt = data.cTime[i] - data.cTime[i-1]
        data.ib_lag[i] = IbLag.calculate_tau(float(data.ib[i]), i == 0, dt, lag_tau)
    return data


# Add reshaped voc_soc curve so it shows on plots
def add_voc_soc_new(data):
    chm = data.chm[1]
    chem = Chemistry(chm)
    chem.assign_all_mod(chm)
    n = len(data.cTime)
    data.voc_soc_new = np.zeros(n)
    for i in range(n):
        data.voc_soc_new[i] = chem.lut_voc_soc.interp(data.soc[i], data.Tb[i])
    return data


def seek_tensor(data_file_path=None, unit_key=None, time_end_in=None, save_pdf_path='./figures', path_to_temp='./temp'):
    date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
    date_ = datetime.now().strftime("%y%m%d")
    if not os.path.isdir(save_pdf_path):
        os.mkdir(save_pdf_path)
    if not os.path.isdir(path_to_temp):
        os.mkdir(path_to_temp)

    # Transient  inputs
    t_ib_fail = None
    init_time_in = None
    use_ib_mon_in = False
    scale_in = None
    use_vb_raw = False
    scale_r_ss_in = 1.
    s_hys_in = 1.
    dvoc_sim_in = 0.
    dvoc_mon_in = 0.
    Bmon_in = None
    Bsim_in = None
    skip = 1
    zero_zero_in = False
    drive_ekf_in = False
    time_end_in = None
    dTb = None
    plot_init_in = False
    long_term_in = False
    plot_overall_in = True
    use_vb_sim_in = False
    sres0_in = 1.
    sresct_in = 1.
    stauct_in = 1
    s_hys_cap_in = 1.
    s_coul_eff_in = 1.
    s_cap_chg_in = 1.
    s_cap_dis_in = 1.
    s_hys_chg_in = 1.
    s_hys_dis_in = 1.
    tune_in = False
    cc_dif_tol_in = 0.2
    verbose_in = False
    legacy_in = False
    cutback_gain_sclr_in = 1.
    ds_voc_soc_in = 0.
    data_file_txt = None
    temp_file = None
    sat_init_in = None
    use_mon_soc_in = True

    # Save these examples
    data_file_txt = 'dv_20230831_soc0p_ch_clip.csv'; unit_key = 'soc0p'; data_file_path = None; use_ib_mon_in = True; zero_zero_in = True;
    # data_file_txt = 'dv_train_soc0p_ch_clip.csv'; unit_key = 'soc0p'; data_file_path = None; use_ib_mon_in = True; zero_zero_in = True; # time_end_in = 248500.
    # data_file_txt = 'dv_validate_soc0p_ch_clip.csv'; unit_key = 'soc0p'; data_file_path = None; use_ib_mon_in = True; zero_zero_in = True
    # data_file_txt = 'dv_test_soc0p_ch.csv'; unit_key = 'soc0p'; data_file_path = None; use_ib_mon_in = True; zero_zero_in = True

    # data_file_txt = 'dv_20230826_soc0p_ch_clip.csv'; unit_key = 'soc0p'; data_file_path = None; use_ib_mon_in = True; zero_zero_in = True;
    # data_file_txt = 'dv_train_soc0p_ch_clip01.csv'; unit_key = 'soc0p'; data_file_path = None; use_ib_mon_in = True; zero_zero_in = True; # time_end_in = 248500.
    # data_file_txt = 'dv_train_soc0p_ch_clip02.csv'; unit_key = 'soc0p'; data_file_path = None; use_ib_mon_in = True; zero_zero_in = True; # time_end_in = 248500.
    # data_file_txt = 'dv_train_soc0p_ch_clip03.csv'; unit_key = 'soc0p'; data_file_path = None; use_ib_mon_in = True; zero_zero_in = True; # time_end_in = 248500.
    # data_file_txt = 'dv_train_soc0p_ch_clip04.csv'; unit_key = 'soc0p'; data_file_path = None; use_ib_mon_in = True; zero_zero_in = True; # time_end_in = 248500.
    # data_file_txt = 'GenerateDV_Data.csv'; unit_key = 'soc0p'; data_file_path = None; zero_zero_in = True; use_vb_sim_in = True

    if data_file_path is None:
        if data_file_txt is not None:
            path_to_data = os.path.join(os.getcwd(), data_file_txt)
        else:
            path_to_data = None
        data_file_path = path_to_data
        if temp_file is None:
            data_file = data_file_path
        else:
            path_to_temp = os.path.join(path_to_temp, temp_file)
            data_file = path_to_temp

    # # Load mon v4 (old)
    mon_old, sim_old, f, data_file_clean, temp_flt_file_clean = \
        load_data(data_file, skip, unit_key, zero_zero_in, time_end_in, legacy=legacy_in)
    mon_old = add_ib_lag(mon_old)
    mon_old = add_voc_soc_new(mon_old)
    mon_old_file_save = data_file_clean.replace(".csv", "_clean.csv")
    save_clean_file(mon_old, mon_old_file_save, 'mon' + date_)

    # How to initialize
    if mon_old.time[0] == 0.:  # no initialization flat detected at beginning of recording
        if init_time_in:
            init_time = init_time_in
        else:
            init_time = 1.
    else:
        if init_time_in:
            init_time = init_time_in
        else:
            init_time = -4.
    # Get dv_hys from data
    # dv_hys = mon_old.dv_hys[0]

    # New run
    mon_file_save = data_file_clean.replace(".csv", "_rep.csv")
    mon_ver, sim_ver, sim_s_ver, mon, sim = \
        replicate(mon_old, sim_old=sim_old, init_time=init_time, sres0=sres0_in, sresct=sresct_in,
                  t_ib_fail=t_ib_fail, use_ib_mon=use_ib_mon_in, scale_in=scale_in, use_vb_raw=use_vb_raw,
                  scale_r_ss=scale_r_ss_in, s_hys_sim=s_hys_in, s_hys_mon=s_hys_in, dvoc_sim=dvoc_sim_in,
                  dvoc_mon=dvoc_mon_in, Bmon=Bmon_in, Bsim=Bsim_in, drive_ekf=drive_ekf_in,
                  dTb_in=dTb, verbose=verbose_in, use_vb_sim=use_vb_sim_in, scale_hys_cap_sim=s_hys_cap_in,
                  scale_hys_cap_mon=s_hys_cap_in, stauct_mon=stauct_in, stauct_sim=stauct_in,
                  s_coul_eff=s_coul_eff_in, s_cap_chg=s_cap_chg_in, s_cap_dis=s_cap_dis_in, s_hys_chg=s_hys_chg_in,
                  s_hys_dis=s_hys_dis_in, cutback_gain_sclr=cutback_gain_sclr_in, ds_voc_soc=ds_voc_soc_in,
                  use_mon_soc=use_mon_soc_in)
    save_clean_file(mon_ver, mon_file_save, 'mon_rep' + date_)

    # Plots
    n_fig = 0
    fig_files = []
    data_root_test = data_file_clean.split('/')[-1].replace('.csv', '')
    dir_root_test = data_file_clean.split('/')[-3].split('\\')[-1]
    filename = data_root_test
    plot_title = dir_root_test + '/' + data_root_test + '   ' + date_time
    if temp_flt_file_clean and len(f.time) > 1:
        n_fig, fig_files = over_fault(f, filename, fig_files=fig_files, plot_title=plot_title, subtitle='faults',
                                      n_fig=n_fig, long_term=long_term_in, cc_dif_tol=cc_dif_tol_in)
    if plot_overall_in:
        n_fig, fig_files = dom_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                    plot_title=plot_title, n_fig=n_fig, plot_init_in=plot_init_in, ref_str='',
                                    test_str='_ver')
        n_fig, fig_files = ekf_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                    plot_title=plot_title, n_fig=n_fig, plot_init_in=plot_init_in, ref_str='',
                                    test_str='_ver')
        n_fig, fig_files = sim_s_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                      plot_title=plot_title, n_fig=n_fig, plot_init_in=plot_init_in, ref_str='',
                                      test_str='_ver')
        n_fig, fig_files = gp_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                   plot_title=plot_title, n_fig=n_fig, plot_init_in=plot_init_in, ref_str='',
                                   test_str='_ver')
        n_fig, fig_files = off_on_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                       plot_title=plot_title, n_fig=n_fig, plot_init_in=plot_init_in, ref_str='',
                                       test_str='_ver')
    if tune_in:
        n_fig, fig_files = tune_r(mon_old, mon_ver, sim_s_ver, filename, fig_files,
                                  plot_title=plot_title, n_fig=n_fig, ref_str='', test_str='_ver')

    precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=save_pdf_path)
    print('filename', filename, 'path', save_pdf_path)
    unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', save_pdf_path=save_pdf_path)
    cleanup_fig_files(fig_files)

    plt.show()
    return True


if __name__ == '__main__':
    seek_tensor()
