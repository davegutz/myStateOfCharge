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

""" Python model of what's installed on the Particle Photon.  Includes
a monitor object (MON) and a simulation object (SIM).   The monitor is
the EKF and Coulomb Counter.   The SIM is a battery model, that also has a
Coulomb Counter built in."""


if __name__ == '__main__':
    import numpy as np
    import sys
    from Battery import overall_batt
    from MonSim import replicate, save_clean_file, save_clean_file_sim
    from DataOverModel import SavedData, SavedDataSim, write_clean_file, overall
    from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
    from CompareHist_20221028 import add_stuff_f, over_fault, filter_Tb, X_SOC_MIN_BB, T_SOC_MIN_BB, IB_BAND, RATED_BATT_CAP, over_easy
    import matplotlib.pyplot as plt
    plt.rcParams['axes.grid'] = True
    from datetime import datetime, timedelta
    from pyDAGx import myTables
    global mon_old

    # Battleborn Bmon=0, Bsim=0
    t_x_soc0 = [-0.15, 0.00, 0.05, 0.10, 0.14, 0.17, 0.20, 0.25, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 0.99, 0.995,
                1.00]
    t_y_t0 = [5., 11.1, 20., 30., 40.]
    t_voc0 = [4.00, 4.00, 9.00, 11.80, 12.50, 12.60, 12.67, 12.76, 12.82, 12.93, 12.98, 13.03, 13.07, 13.11, 13.17,
              13.22, 13.59, 14.45,
              4.00, 4.00, 10.00, 12.30, 12.60, 12.65, 12.71, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20,
              13.23, 13.60, 14.46,
              4.00, 12.22, 12.66, 12.74, 12.80, 12.85, 12.89, 12.95, 12.99, 13.05, 13.08, 13.13, 13.18, 13.21, 13.25,
              13.27, 13.72, 14.50,
              4.00, 4.00, 12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26,
              13.27, 13.72, 14.50,
              4.00, 4.00, 4.00, 4.00, 10.50, 11.93, 12.78, 12.83, 12.89, 12.97, 13.06, 13.10, 13.13, 13.16, 13.19,
              13.20, 13.72, 14.50]
    x0 = np.array(t_x_soc0)
    y0 = np.array(t_y_t0)
    data_interp0 = np.array(t_voc0)
    lut_voc = myTables.TableInterp2D(x0, y0, data_interp0)
    t_x_soc_min = [5., 11.1, 20., 30., 40.]
    t_soc_min = [0.07, 0.05, -0.05, 0.00, 0.20]
    lut_soc_min = myTables.TableInterp1D(np.array(t_x_soc_min), np.array(t_soc_min))


    def main():
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        date_ = datetime.now().strftime("%y%m%d")

        # Transient  inputs
        t_Ib_fail = None
        init_time_in = None
        use_ib_mon_in = False
        scale_in = None
        use_Vb_raw = False
        unit_key = None
        data_file_old_txt = None
        scale_r_ss_in = 1.
        scale_hys_sim_in = 1.
        scale_hys_mon_in = 1.
        dvoc_sim_in = 0.
        dvoc_mon_in = 0.
        Bmon_in = None
        Bsim_in = None
        skip = 1
        zero_zero_in = False
        drive_ekf_in = False
        time_end_in = None
        unit_key = 'pro_2022'
        dTb = None
        plot_init_in = False
        long_term_in = False
        plot_overall_in = True

        # Save these
        # data_file_old_txt = '../dataReduction/real world Xp20 20220902.txt'; unit_key = 'soc0_2022'; use_ib_mon_in=True; scale_in=1.12

        # Regression suite
        # data_file_old_txt = 'ampHiFail v20221028.txt'
        # data_file_old_txt = 'ampHiFailFf v20221028.txt'
        # data_file_old_txt = 'ampLoFail v20221028.txt'
        # data_file_old_txt = 'ampHiFailNoise v20221028.txt'
        # data_file_old_txt = 'rapidTweakRegression v20221028.txt'  # ; time_end_in=4.8;
        # data_file_old_txt = 'rapidTweakRegression40C v20221028.txt'  # ; time_end_in=4.8;
        # data_file_old_txt = 'slowTweakRegression v20221028.txt'
        # data_file_old_txt = 'triTweakDisch v20221028.txt'  #; time_end_in=25.4
        # data_file_old_txt = 'satSit v20221028.txt'
        # data_file_old_txt = 'satSitHys v20221028.txt'
        # data_file_old_txt = 'offSitHysBms v20221028.txt'  # ; time_end_in = 137.
        # data_file_old_txt = 'offSitHysBmsNoise v20221028.txt'  # ; time_end_in=50
        # data_file_old_txt = 'ampHiFailSlow v20221028.txt'  # ; time_end_in=360
        # data_file_old_txt = 'vHiFail v20221028.txt'
        # data_file_old_txt = 'pulseEKF v20221028.txt'; init_time_in=-0.001;  # TODO:  broken
        # data_file_old_txt = 'pulseSS v20221028.txt'; init_time_in=-0.001;
        # data_file_old_txt = 'tbFailMod v20221028.txt'
        # data_file_old_txt = 'tbFailHdwe v20221028.txt'
        # data_file_old_txt = 'EKF_Track v20221028.txt'
        # data_file_old_txt = 'EKF_Track Dr2000 v20221028.txt'
        # data_file_old_txt = 'on_off_on v20221028.txt'  # ; time_end_in=6
        data_file_old_txt = 'dwell noise Ca.5 v20221028.txt'  # ; dTb = [[0., 18000.],  [0, 8.]]
        # data_file_old_txt = 'dwell Ca.5 v20221028.txt'  # ; time_end_in=0.5  # ; dTb = [[0., 18000.],  [0, 8.]]
        #
        # data_file_old_txt = 'fail 20221124.txt';  plot_overall_in=False;  # ; long_term_in=True;
        # data_file_old_txt = 'init Ca1 v20220926.txt'
        # data_file_old_txt = 'real world Xp20 30C 20220914.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_Vb_raw = False; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; scale_hys_sim_in = 3.33; dvoc_mon_in = -0.05; dvoc_sim_in = -0.05
        # data_file_old_txt = 'real world Xp20 30C 20220914a+b.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_Vb_raw = False; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; scale_hys_sim_in = 3.33; dvoc_mon_in = -0.05; dvoc_sim_in = -0.05
        # data_file_old_txt = 'real world Xp20 30C 20220917.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; init_time_in = -11110
        # data_file_old_txt = 'gorilla v20220917a.txt'
        # data_file_old_txt = 'EKF_Track Dr2000 fault v20220917.txt'
        # data_file_old_txt = 'real world Xp20 v20220917a.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; init_time_in = -69900
        # data_file_old_txt = 'weird v20220917d.txt'; unit_key = 'soc0_2022'; scale_in = 1.084
        # data_file_old_txt = 'dwell noise Ca.5 v20220926.txt'#; dTb = [[0., 18000.],  [0, 8.]]
        title_key = "unit,"  # Find one instance of title
        title_key_sel = "unit_s,"  # Find one instance of title
        unit_key_sel = "unit_sel"
        title_key_ekf = "unit_e,"  # Find one instance of title
        unit_key_ekf = "unit_ekf"
        title_key_sim = "unit_m,"  # Find one instance of title
        unit_key_sim = "unit_sim"
        temp_flt_file = 'flt_compareRun.txt'
        pathToSavePdfTo = '../dataReduction/figures'
        path_to_data = '../dataReduction'
        path_to_temp = '../dataReduction/temp'

        # Load mon v4 (old)
        data_file_clean = write_clean_file(data_file_old_txt, type_='_mon', title_key=title_key, unit_key=unit_key,
                                           skip=skip, path_to_data=path_to_data, path_to_temp=path_to_temp)
        cols = ('cTime', 'dt', 'chm', 'sat', 'sel', 'mod', 'bmso', 'Tb', 'Vb', 'Ib', 'Ib_charge', 'ioc', 'voc_soc',
                'Vsat', 'dV_dyn', 'Voc_stat', 'Voc_ekf', 'y_ekf', 'soc_s', 'soc_ekf', 'soc')
        mon_old_raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols,  dtype=float,
                                    encoding=None).view(np.recarray)

        # Load sel (old)
        sel_file_clean = write_clean_file(data_file_old_txt, type_='_sel', title_key=title_key_sel,
                                          unit_key=unit_key_sel, skip=skip, path_to_data=path_to_data,
                                          path_to_temp=path_to_temp)
        cols_sel = ('c_time', 'res', 'user_sel', 'cc_dif',
                    'ibmh', 'ibnh', 'ibmm', 'ibnm', 'ibm', 'ib_diff', 'ib_diff_f',
                    'voc_soc', 'e_w', 'e_w_f',
                    'ib_sel', 'Ib_h', 'Ib_s', 'mib', 'Ib',
                    'vb_sel', 'Vb_h', 'Vb_s', 'mvb', 'Vb',
                    'Tb_h', 'Tb_s', 'mtb', 'Tb_f',
                    'fltw', 'falw', 'ib_rate', 'ib_quiet', 'tb_sel',
                    'ccd_thr', 'ewh_thr', 'ewl_thr', 'ibd_thr', 'ibq_thr', 'preserving')
        sel_old_raw = None
        if sel_file_clean:
            sel_old_raw = np.genfromtxt(sel_file_clean, delimiter=',', names=True, usecols=cols_sel, dtype=float,
                                        encoding=None).view(np.recarray)

        # Load ekf (old)
        ekf_file_clean = write_clean_file(data_file_old_txt, type_='_ekf', title_key=title_key_ekf,
                                          unit_key=unit_key_ekf, skip=skip, path_to_data=path_to_data,
                                          path_to_temp=path_to_temp)
        cols_ekf = ('c_time', 'Fx_', 'Bu_', 'Q_', 'R_', 'P_', 'S_', 'K_', 'u_', 'x_', 'y_', 'z_', 'x_prior_',
                    'P_prior_', 'x_post_', 'P_post_', 'hx_', 'H_')
        ekf_old_raw = None
        if ekf_file_clean:
            ekf_old_raw = np.genfromtxt(ekf_file_clean, delimiter=',', names=True, usecols=cols_ekf, dtype=float,
                                        encoding=None).view(np.recarray)
        mon_old = SavedData(data=mon_old_raw, sel=sel_old_raw, ekf=ekf_old_raw, time_end=time_end_in,
                            zero_zero=zero_zero_in)

        # Load sim _s v24 portion of real-time run (old)
        data_file_sim_clean = write_clean_file(data_file_old_txt, type_='_sim', title_key=title_key_sim,
                                               unit_key=unit_key_sim, skip=skip, path_to_data=path_to_data,
                                               path_to_temp=path_to_temp)
        cols_sim = ('c_time', 'chm_s', 'bmso_s', 'Tb_s', 'Tbl_s', 'vsat_s', 'voc_stat_s', 'dv_dyn_s', 'vb_s', 'ib_s',
                    'ib_in_s', 'ib_charge_s', 'ioc_s', 'sat_s', 'dq_s', 'soc_s', 'reset_s')
        if data_file_sim_clean:
            sim_old_raw = np.genfromtxt(data_file_sim_clean, delimiter=',', names=True, usecols=cols_sim,
                                        dtype=float, encoding=None).view(np.recarray)
            sim_old = SavedDataSim(time_ref=mon_old.time_ref, data=sim_old_raw, time_end=time_end_in)
        else:
            sim_old = None

        # Load fault
        temp_flt_file_clean = write_clean_file(data_file_old_txt, type_='_flt', title_key='fltb', unit_key='unit_f',
                                               skip=skip, path_to_data=path_to_data, path_to_temp=path_to_temp,
                                               comment_str='---')
        cols_f = ('time', 'Tb_h', 'vb_h', 'ibah', 'ibnh', 'Tb', 'vb', 'ib', 'soc', 'soc_ekf', 'voc', 'Voc_stat',
                  'e_w_f', 'fltw', 'falw')
        if temp_flt_file_clean:
            f_raw = np.genfromtxt(temp_flt_file_clean, delimiter=',', names=True, usecols=cols_f, dtype=None,
                                  encoding=None).view(np.recarray)
        else:
            print("data from", temp_flt_file, "empty after loading")
        if temp_flt_file_clean:
            f_raw = np.unique(f_raw)
            f = add_stuff_f(f_raw, voc_soc_tbl=lut_voc, soc_min_tbl=lut_soc_min, ib_band=IB_BAND)
            print("\nf:\n", f, "\n")
            f = filter_Tb(f, 20., tb_band=100., rated_batt_cap=RATED_BATT_CAP)

        # How to initialize
        if mon_old.time[0] == 0.:  # no initialization flat detected at beginning of recording
            init_time = 1.
        else:
            if init_time_in:
                init_time = init_time_in
            else:
                init_time = -4.
        # Get dv_hys from data
        dv_hys = mon_old.dV_hys[0]

        # New run
        mon_file_save = data_file_clean.replace(".csv", "_rep.csv")
        mon_ver, sim_ver, randles_ver, sim_s_ver = replicate(mon_old, sim_old=sim_old, init_time=init_time,
                                                             sres=1.0, t_Ib_fail=t_Ib_fail, use_ib_mon=use_ib_mon_in,
                                                             scale_in=scale_in, use_Vb_raw=use_Vb_raw,
                                                             scale_r_ss=scale_r_ss_in, s_hys_sim=scale_hys_sim_in,
                                                             s_hys_mon=scale_hys_mon_in, dvoc_sim=dvoc_sim_in,
                                                             dvoc_mon=dvoc_mon_in, Bmon=Bmon_in, Bsim=Bsim_in,
                                                             drive_ekf=drive_ekf_in, dTb_in=dTb, verbose=False)
        save_clean_file(mon_ver, mon_file_save, 'mon_rep' + date_)

        # Plots
        n_fig = 0
        fig_files = []
        data_root = data_file_clean.split('/')[-1].replace('.csv', '-')
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        if temp_flt_file_clean and len(f.time) > 1:
            n_fig, fig_files = over_fault(f, filename, fig_files=fig_files, plot_title=plot_title, subtitle='faults',
                                          n_fig=n_fig, x_sch=X_SOC_MIN_BB, z_sch=T_SOC_MIN_BB, voc_reset=0.,
                                          long_term=long_term_in)
        # if temp_hist_file_clean and len(h.time) > 1:
        #         n_fig, fig_files = over_easy(h_20C, filename, mv_fast=mon_ver_300new, mv_slow=mon_ver_300old,
        #                                  fig_files=fig_files, plot_title=plot_title, subtitle='h_20C',
        #                                  n_fig=n_fig, x_sch=x0, z_sch=voc_soc20, voc_reset=VOC_RESET_20)
        # n_fig, fig_files = overall_batt(mon_ver, sim_ver, randles_ver, filename, fig_files, plot_title=plot_title,
        #                                 n_fig=n_fig, suffix='_ver')  # sim over mon verify
        if plot_overall_in:
            n_fig, fig_files = overall(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                       plot_title=plot_title, n_fig=n_fig, plot_init_in=plot_init_in)  # all over all
        precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=pathToSavePdfTo)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo=pathToSavePdfTo)
        cleanup_fig_files(fig_files)

        plt.show()

    main()
