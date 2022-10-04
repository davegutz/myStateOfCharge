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
    import matplotlib.pyplot as plt
    plt.rcParams['axes.grid'] = True
    from datetime import datetime, timedelta
    global mon_old

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
        # Save these
        # data_file_old_txt = '../dataReduction/real world Xp20 20220902.txt'; unit_key = 'soc0_2022'; use_ib_mon_in=True; scale_in=1.12

        # Regression suite
        # data_file_old_txt = 'ampHiFail v20220926.txt'
        # data_file_old_txt = 'ampLoFail20220914.txt'
        # data_file_old_txt = 'ampHiFailNoise20220914.txt'
        data_file_old_txt = 'rapidTweakRegression v20220926.txt'
        # data_file_old_txt = 'rapidTweakRegression40C_20220914.txt'
        # data_file_old_txt = 'slowTweakRegression20220914.txt'
        # data_file_old_txt = 'triTweakDisch v20220917a.txt'
        # data_file_old_txt = 'satSit20220926.txt'
        # data_file_old_txt = 'satSitHys20220926.txt'; #time_end_in=50
        # data_file_old_txt = 'offSitHys20220926.txt'; #time_end_in=50
        # data_file_old_txt = 'offSitHysBms20220926.txt'; #time_end_in=50
        # data_file_old_txt = 'offSitHysBmsNoise20220926.txt'; #time_end_in=50
        # data_file_old_txt = 'init Ca1 v20220926.txt'
        # data_file_old_txt = 'ampHiFailSlow20220914.txt'
        # data_file_old_txt = 'vHiFail v20220917a.txt'
        # data_file_old_txt = 'pulse20220914.txt'; init_time_in=-0.001;
        # data_file_old_txt = 'tbFailMod20220914.txt'
        # data_file_old_txt = 'tbFailHdwe20220914.txt'
        # data_file_old_txt = 'real world Xp20 30C 20220914.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_Vb_raw = False; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; scale_hys_sim_in = 3.33; dvoc_mon_in = -0.05; dvoc_sim_in = -0.05
        # data_file_old_txt = 'real world Xp20 30C 20220914a+b.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_Vb_raw = False; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; scale_hys_sim_in = 3.33; dvoc_mon_in = -0.05; dvoc_sim_in = -0.05
        # data_file_old_txt = 'real world Xp20 30C 20220917.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; init_time_in = -11110
        # data_file_old_txt = 'EKF_Track v20220917.txt'
        # data_file_old_txt = 'EKF_Track Dr2000 v20220917.txt'
        # data_file_old_txt = 'gorilla v20220917a.txt'
        # data_file_old_txt = 'on_off_on v20220917a.txt'
        # data_file_old_txt = 'start_up v20220917a.txt'
        # data_file_old_txt = 'EKF_Track Dr2000 fault v20220917.txt'
        # data_file_old_txt = 'real world Xp20 v20220917a.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; init_time_in = -69900
        # data_file_old_txt = 'weird v20220917d.txt'; unit_key = 'soc0_2022'; scale_in = 1.084
        title_key = "unit,"  # Find one instance of title
        title_key_sel = "unit_s,"  # Find one instance of title
        unit_key_sel = "unit_sel"
        title_key_ekf = "unit_e,"  # Find one instance of title
        unit_key_ekf = "unit_ekf"
        title_key_sim = "unit_m,"  # Find one instance of title
        unit_key_sim = "unit_sim"
        pathToSavePdfTo = '../dataReduction/figures'
        path_to_data = '../dataReduction'
        path_to_temp = '../dataReduction/temp'

        # Load mon v4 (old)
        data_file_clean = write_clean_file(data_file_old_txt, type_='_mon', title_key=title_key, unit_key=unit_key,
                                           skip=skip, path_to_data=path_to_data, path_to_temp=path_to_temp)
        cols = ('cTime', 'dt', 'chm', 'sat', 'sel', 'mod', 'Tb', 'Vb', 'Ib', 'ioc', 'voc_soc', 'Vsat', 'dV_dyn', 'Voc_stat',
                'Voc_ekf', 'y_ekf', 'soc_s', 'soc_ekf', 'soc')
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
                    'ccd_thr', 'ewh_thr', 'ewl_thr', 'ibd_thr', 'ibq_thr')
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
        cols_sim = ('c_time', 'chm_s', 'Tb_s', 'Tbl_s', 'vsat_s', 'voc_stat_s', 'dv_dyn_s', 'vb_s', 'ib_s',
                    'ib_in_s', 'ioc_s', 'sat_s', 'dq_s', 'soc_s', 'reset_s')
        if data_file_sim_clean:
            sim_old_raw = np.genfromtxt(data_file_sim_clean, delimiter=',', names=True, usecols=cols_sim,
                                        dtype=float, encoding=None).view(np.recarray)
            sim_old = SavedDataSim(time_ref=mon_old.time_ref, data=sim_old_raw, time_end=time_end_in)
        else:
            sim_old = None

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
                                                             dv_hys=dv_hys, sres=1.0, t_Ib_fail=t_Ib_fail,
                                                             use_ib_mon=use_ib_mon_in, scale_in=scale_in,
                                                             use_Vb_raw=use_Vb_raw, scale_r_ss=scale_r_ss_in,
                                                             s_hys_sim=scale_hys_sim_in, s_hys_mon=scale_hys_mon_in,
                                                             dvoc_sim=dvoc_sim_in, dvoc_mon=dvoc_mon_in,
                                                             Bmon=Bmon_in, Bsim=Bsim_in, drive_ekf=drive_ekf_in)
        save_clean_file(mon_ver, mon_file_save, 'mon_rep' + date_)

        # Plots
        n_fig = 0
        fig_files = []
        data_root = data_file_clean.split('/')[-1].replace('.csv', '-')
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        # n_fig, fig_files = overall_batt(mon_ver, sim_ver, randles_ver, filename, fig_files, plot_title=plot_title,
        #                                 n_fig=n_fig, suffix='_ver')  # sim over mon verify
        n_fig, fig_files = overall(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                   plot_title=plot_title, n_fig=n_fig)  # all over all
        precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=pathToSavePdfTo)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo=pathToSavePdfTo)
        cleanup_fig_files(fig_files)

        plt.show()

    main()
