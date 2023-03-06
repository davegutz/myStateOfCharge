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


if __name__ == '__main__':
    import sys
    from MonSim import replicate, save_clean_file
    from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
    from CompareFault import over_fault
    import matplotlib.pyplot as plt
    plt.rcParams['axes.grid'] = True
    from datetime import datetime
    from CompareRunRun import load_data
    from DataOverModel import dom_plot
    from PlotSimS import sim_s_plot
    from PlotEKF import ekf_plot
    from PlotGP import tune_r, gp_plot
    from PlotOffOn import off_on_plot

    def main():
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        date_ = datetime.now().strftime("%y%m%d")
        pathToSavePdfTo = '../dataReduction/figures'
        path_to_data = '../dataReduction'
        path_to_temp = '../dataReduction/temp'

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

        # Save these
        # data_file_old_txt = '../dataReduction/real world Xp20 20220902.txt'; unit_key = 'soc0_2022'; use_ib_mon_in=True; scale_in=1.12

        # Regression suite

        # data_file_old_txt = 'ampHiFail v20230219.txt'; unit_key = 'pro0p'
        # data_file_old_txt = 'ampHiFail v20230303 CH.txt'; unit_key = 'pro0p'; scale_in = 1.127; cc_dif_tol_in = 0.5; legacy_in = True
        # data_file_old_txt = 'ampHiFail v20230305 CH.txt'; unit_key = 'pro0p'; cc_dif_tol_in = 0.5
        # data_file_old_txt = 'ampHiFail vA20230305 BB.txt'; unit_key = 'pro1a'
        # data_file_old_txt = 'ampHiFail vA20221220.txt';  unit_key = 'soc1a'

        # data_file_old_txt = 'rapidTweakRegression v20230303 CH.txt'; unit_key = 'pro0p_2023';  scale_in = 1.127; cc_dif_tol_in = 0.5
        # data_file_old_txt = 'flatSit v20230303 CH.txt'; unit_key = 'pro0p_2023';  scale_in = 1.127; cc_dif_tol_in = 0.5
        # data_file_old_txt = 'rapidTweakRegression vA20221220.txt'; unit_key = 'soc1a'  # ; time_end_in = 4.8;
        # data_file_old_txt = 'rapidTweakRegression vA20230217.txt'; unit_key = 'pro1a'
        # data_file_old_txt = 'rapidTweakRegression vA20230219.txt'; unit_key = 'pro1a'
        # data_file_old_txt = 'rapidTweakRegression vA20230305 BB.txt'; unit_key = 'pro1a'
        # data_file_old_txt = 'rapidTweakRegression v20230305 CH.txt'; unit_key = 'pro0p'

        # data_file_old_txt = 'offSitHysBms v20221220.txt'; unit_key = 'pro0p'   # ; time_end_in = 137.
        # data_file_old_txt = 'offSitHysBms vA20221220.txt'; unit_key = 'soc1a'  #; time_end_in = 10.
        # data_file_old_txt = 'offSitHysBms v20230303 CH.txt'; unit_key = 'pro0p'; scale_in = 1.127; cc_dif_tol_in = 0.5  #; time_end_in = 159
        # data_file_old_txt = 'offSitHysBms vA20230305 BB.txt'; unit_key = 'pro1a'
        # data_file_old_txt = 'offSitHysBms v20230305 CH.txt'; unit_key = 'pro0p'

        # data_file_old_txt = 'Xm0VbFail.txt'; unit_key = 'soc1a'  #; time_end_in = 10.

        # data_file_old_txt = 'triTweakDisch v20221220.txt'  #; time_end_in=25.4
        # data_file_old_txt = 'triTweakDisch v20230219.txt'; unit_key = 'pro0p'; scale_in = 1.127; cc_dif_tol_in = 0.5  #;time_end_in = 2.
        # data_file_old_txt = 'triTweakDisch v20230303 CH.txt'; unit_key = 'pro0p'; scale_in = 1.127; cc_dif_tol_in = 0.5  # ; time_end_in = 35.
        # data_file_old_txt = 'triTweakDisch vA20230305 BB.txt'; unit_key = 'pro1a'
        # data_file_old_txt = 'triTweakDisch v20230305 CH.txt'; unit_key = 'pro0p'

        data_file_old_txt = 'coldStart vA20230305 BB.txt'; unit_key = 'pro1a'
        # data_file_old_txt = 'coldStart v20221220.txt'  #; time_end_in=112

        # data_file_old_txt = 'ampHiFailFf v20221028.txt'
        # data_file_old_txt = 'ampHiFailFf v20230303 CH.txt'; unit_key = 'pro0p_2023';  scale_in = 1.127; cc_dif_tol_in = 0.5
        # data_file_old_txt = 'ampHiFailFf vA20230305 BB.txt'; unit_key = 'pro1a'
        # data_file_old_txt = 'ampHiFailFf v20230305 CH.txt'; unit_key = 'pro0p'

        # data_file_old_txt = 'ampLoFail v20221028.txt'
        # data_file_old_txt = 'ampLoFail v20230303 CH.txt'; unit_key = 'pro0p_2023';  scale_in = 1.127; cc_dif_tol_in = 0.5
        # data_file_old_txt = 'ampLoFail vA20230305 BB.txt'; unit_key = 'pro1a'
        # data_file_old_txt = 'ampLoFail v20230305 CH.txt'; unit_key = 'pro0p'

        # data_file_old_txt = 'ampHiFailNoise v20221028.txt'
        # data_file_old_txt = 'ampHiFailNoise vA20230305 BB.txt'; unit_key = 'pro1a'
        # data_file_old_txt = 'ampHiFailNoise v20230305 CH.txt'; unit_key = 'pro0p'

        # data_file_old_txt = 'rapidTweakRegression40C v20221028.txt'  # ; time_end_in=4.8;
        # data_file_old_txt = 'rapidTweakRegression40C vA20230305 BB.txt'; unit_key = 'pro1a'
        # data_file_old_txt = 'rapidTweakRegression40C v20230305 CH.txt'; unit_key = 'pro0p'

        # data_file_old_txt = 'slowTweakRegression v20221028.txt'

        # data_file_old_txt = 'satSit v20221028.txt'
        # data_file_old_txt = 'satSitHys v20221028.txt'

        # data_file_old_txt = 'offSitHysBmsNoise v20221028.txt'  # ; time_end_in=50

        # data_file_old_txt = 'ampHiFailSlow v20221028.txt'  # ; time_end_in=360

        # data_file_old_txt = 'vHiFail v20221028.txt'
        # data_file_old_txt = 'vHiFail v20230303 CH.txt'; unit_key = 'pro0p_2023';  scale_in = 1.127; cc_dif_tol_in = 0.5

        # data_file_old_txt = 'vHiFailFf v20230303 CH.txt'; unit_key = 'pro0p_2023';  scale_in = 1.127; cc_dif_tol_in = 0.5

        # data_file_old_txt = 'pulseEKF v20221028.txt'; init_time_in=-0.001;  # TODO:  broken

        # data_file_old_txt = 'pulseSS v20221028.txt'; init_time_in=-0.001;
        # data_file_old_txt = 'pulseSS v20230303 CH.txt'; unit_key = 'pro0p_2023';  scale_in = 1.127; cc_dif_tol_in = 0.5; verbose_in = True

        # data_file_old_txt = 'tbFailMod v20221220.txt'
        # data_file_old_txt = 'tbFailMod v20230303 CH.txt'; unit_key = 'pro0p_2023';  scale_in = 1.127; cc_dif_tol_in = 0.5

        # data_file_old_txt = 'tbFailHdwe v20221028.txt'
        # data_file_old_txt = 'tbFailHdwe v20230303 CH.txt'; unit_key = 'pro0p_2023';  scale_in = 1.127; cc_dif_tol_in = 0.5

        # data_file_old_txt = 'EKF_Track v20221028.txt'
        # data_file_old_txt = 'EKF_Track Dr2000 v20221028.txt'

        # data_file_old_txt = 'on_off_on v20221028.txt'  # ; time_end_in=6

        # data_file_old_txt = 'dwell noise Ca.5 v20221028.txt'  # ; dTb = [[0., 18000.],  [0, 8.]]
        # data_file_old_txt = 'dwell Ca.5 v20221028.txt'  # ; time_end_in=0.5  # ; dTb = [[0., 18000.],  [0, 8.]]

        # The following CHINS runs were sources of figures in the .odt report.  Chem is in data stream
        # in the following line I forgot to renom sp on load so scale_hys was 1.5 by mistake for baseline run.
        # data_file_old_txt = 'sat v20230128 20230201.txt'; unit_key = 'soc0p';  scale_in = 1.127; s_hys_in = 1.5; s_hys_in = 1.15; #stauct_in=0.1; s_hys_cap_in=1.;
        # data_file_old_txt = 'steps v20230128 20230203.txt'; unit_key = 'soc0p';  scale_in = 1.127; sres0_in = 1; sresct_in = 1; stauct_in = 1; s_hys_chg_in = 1; s_hys_dis_in = 1; s_cap_chg_in = 1.; s_cap_dis_in = 1.; tune_in = True # 0.9 tune 4, 5
        # data_file_old_txt = 'steps v20230128 20230204.txt'; unit_key = 'soc0p';  scale_in = 1.127; sres0_in = 1; sresct_in = 1; stauct_in = 1; s_hys_chg_in = 1; s_hys_dis_in = 1; s_cap_chg_in = 1.; s_cap_dis_in = 1.; tune_in = True  # 0.8 tune 4, 5 set s_hys_chg/dis = 0 to see prediction for R
        # data_file_old_txt = 'steps v20230128 20230214.txt'; unit_key = 'soc0p';  scale_in = 1.127; sres0_in = 1; sresct_in = 1; stauct_in = 1; s_hys_chg_in = 1; s_hys_dis_in = 1; s_cap_chg_in = 1.; s_cap_dis_in = 1.; tune_in = True  # ; time_end_in = 450  # 0.4 tune 4, 5 set s_hys_chg/dis = 0 to see prediction for R
        # data_file_old_txt = 'steps v20230128 20230218.txt'; unit_key = 'soc0p';  scale_in = 1.127; sres0_in = 1; sresct_in = 1; stauct_in = 1; s_hys_chg_in = 1; s_hys_dis_in = 1; s_cap_chg_in = 1.; s_cap_dis_in = 1.; tune_in = True  # ; time_end_in = 450  # 0.1 tune 4, 5 set s_hys_chg/dis = 0 to see prediction for R

        # data_file_old_txt = 'coldCharge v20221028 20221210.txt'; unit_key = 'soc0_2022'; use_vb_sim_in = True
        # data_file_old_txt = 'vb_mess.txt'; unit_key = 'pro1a_2022';
        # data_file_old_txt = 'fail 20221124.txt';  plot_overall_in=False;  # ; long_term_in=True;
        # data_file_old_txt = 'init Ca1 v20220926.txt'
        # data_file_old_txt = 'real world Xp20 30C 20220914.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_vb_raw = False; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; s_hys_in = 3.33; dvoc_mon_in = -0.05; dvoc_sim_in = -0.05
        # data_file_old_txt = 'real world Xp20 30C 20220914a+b.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_vb_raw = False; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; s_hys_in = 3.33; dvoc_mon_in = -0.05; dvoc_sim_in = -0.05
        # data_file_old_txt = 'real world Xp20 30C 20220917.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; init_time_in = -11110
        # data_file_old_txt = 'gorilla v20220917a.txt'
        # data_file_old_txt = 'EKF_Track Dr2000 fault v20220917.txt'
        # data_file_old_txt = 'real world Xp20 v20220917a.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; init_time_in = -69900
        # data_file_old_txt = 'weird v20220917d.txt'; unit_key = 'soc0_2022'; scale_in = 1.084
        # data_file_old_txt = 'dwell noise Ca.5 v20220926.txt'#; dTb = [[0., 18000.],  [0, 8.]]

        # title_key = "unit,"  # Find one instance of title
        # title_key_sel = "unit_s,"  # Find one instance of title
        # unit_key_sel = "unit_sel"
        # title_key_ekf = "unit_e,"  # Find one instance of title
        # unit_key_ekf = "unit_ekf"
        # title_key_sim = "unit_m,"  # Find one instance of title
        # unit_key_sim = "unit_sim"
        # temp_flt_file = 'flt_compareRunSim.txt'

        # # Load mon v4 (old)
        mon_old, sim_old, f, data_file_clean, temp_flt_file_clean = \
            load_data(data_file_old_txt, skip, path_to_data, path_to_temp, unit_key, zero_zero_in, time_end_in,
                      legacy=legacy_in)

        # How to initialize
        if mon_old.time[0] == 0.:  # no initialization flat detected at beginning of recording
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
                      s_hys_dis=s_hys_dis_in)
        save_clean_file(mon_ver, mon_file_save, 'mon_rep' + date_)

        # Plots
        n_fig = 0
        fig_files = []
        data_root = data_file_clean.split('/')[-1].replace('.csv', '-')
        # filename = data_root + sys.argv[0].split('\\')[-1]
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        if temp_flt_file_clean and len(f.time) > 1:
            n_fig, fig_files = over_fault(f, filename, fig_files=fig_files, plot_title=plot_title, subtitle='faults',
                                          n_fig=n_fig, long_term=long_term_in, cc_dif_tol=cc_dif_tol_in)
        if plot_overall_in:
            n_fig, fig_files = dom_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                        plot_title=plot_title, n_fig=n_fig, plot_init_in=plot_init_in, old_str='',
                                        new_str='_ver')
            n_fig, fig_files = ekf_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                        plot_title=plot_title, n_fig=n_fig, plot_init_in=plot_init_in, old_str='',
                                        new_str='_ver')
            n_fig, fig_files = sim_s_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                        plot_title=plot_title, n_fig=n_fig, plot_init_in=plot_init_in, old_str='',
                                        new_str='_ver')
            n_fig, fig_files = gp_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                       plot_title=plot_title, n_fig=n_fig, plot_init_in=plot_init_in, old_str='',
                                       new_str='_ver')
            n_fig, fig_files = off_on_plot(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                           plot_title=plot_title, n_fig=n_fig, plot_init_in=plot_init_in, old_str='',
                                           new_str='_ver')
        if tune_in:
            n_fig, fig_files = tune_r(mon_old, mon_ver, sim_s_ver, filename, fig_files,
                                      plot_title=plot_title, n_fig=n_fig, old_str='', new_str='_ver')

        precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=pathToSavePdfTo)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo=pathToSavePdfTo)
        cleanup_fig_files(fig_files)

        plt.show()

    main()
