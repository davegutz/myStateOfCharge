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
import os
from PlotKiller import show_killer
plt.rcParams['axes.grid'] = True


def compare_run_sim(data_file=None, unit_key=None, time_end_in=None, rel_path_to_save_pdf='./figures',
                    rel_path_to_temp='./temp', data_only=False):

    print(f"{data_file=}\n{rel_path_to_save_pdf=}\n{rel_path_to_temp=}\n")

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
    temp_file = None
    use_vb_raw = False
    dvoc_sim_in = 0.
    dvoc_mon_in = 0.

    # detect running interactively
    # this is written to run in pwd of call
    if data_file is None and temp_file is None:
        path_to_data = easygui.fileopenbox(msg="choose your data file to plot")
        data_file = easygui.filesavebox(msg="pick new file name, cancel to keep", title="get new file name")
        if data_file is None:
            data_file = path_to_data
        else:
            os.rename(path_to_data, data_file)
        unit_key = easygui.enterbox(msg="enter pro0p, pro1a, soc0p, soc1a", title="get unit_key", default="pro1a")
        # Put configurations unique to this build of logic here
        if unit_key == 'pro02p2':
            pass
        elif unit_key == 'pro3p2':
            pass
        elif unit_key == 'pro1a':
            pass
        elif unit_key == 'pro0p':
            pass
        elif unit_key == 'soc0p':
            pass
        elif unit_key == 'soc1a':
            pass
    elif temp_file is not None:
        rel_path_to_temp = os.path.join(rel_path_to_temp, temp_file)
        data_file = rel_path_to_temp

    # Folder operations
    (data_file_folder, data_file_txt) = os.path.split(data_file)
    save_pdf_path = os.path.join(data_file_folder, rel_path_to_save_pdf)
    if not os.path.isdir(rel_path_to_save_pdf):
        os.mkdir(rel_path_to_save_pdf)
    path_to_temp = os.path.join(data_file_folder, rel_path_to_temp)
    if not os.path.isdir(path_to_temp):
        os.mkdir(path_to_temp)

    # # Load mon v4 (old)
    mon_old, sim_old, f, data_file_clean, temp_flt_file_clean = \
        load_data(data_file, 1, unit_key, zero_zero_in, time_end_in, legacy=legacy_in)

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
        replicate(mon_old, sim_old=sim_old, init_time=init_time, use_ib_mon=use_ib_mon_in,
                  use_vb_raw=use_vb_raw, dvoc_sim=dvoc_sim_in, dvoc_mon=dvoc_mon_in, use_vb_sim=use_vb_sim_in,
                  ds_voc_soc=ds_voc_soc_in)
    save_clean_file(mon_ver, mon_file_save, 'mon_rep' + date_)

    # Plots
    if data_only is False:
        fig_list = []
        fig_files = []
        data_root_test = data_file_clean.split('/')[-1].replace('.csv', '')
        dir_root_test = data_file_clean.split('/')[-3].split('\\')[-1]
        filename = data_root_test
        plot_title = dir_root_test + '/' + data_root_test + '   ' + date_time
        if temp_flt_file_clean and len(f.time) > 1:
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

    return mon_old, sim_old, mon_ver, sim_ver, sim_s_ver


if __name__ == '__main__':
    # Save these examples
    # data_file_txt = '../dataReduction/slowTweakRegression_pro0p_ch.csv'; unit_key = 'g20231111_pro0p_ch'; #use_ib_mon_in=True; scale_in=1.12
    # data_file_txt = '../dataReduction/ampHiFail_pro0p_ch.csv'; unit_key = 'g20230530d_pro0p_ch'; #use_ib_mon_in=True; scale_in=1.12
    # data_file_txt = '../dataReduction/real world Xp20 20220902.txt'; unit_key = 'soc0_2022'; use_ib_mon_in=True; scale_in=1.12
    # data_file_txt = 'ampHiFail v20230305 CH.txt'; unit_key = 'pro0p'; cc_dif_tol_in = 0.5
    # data_file_txt = 'ampHiFail vA20230305 BB.txt'; unit_key = 'pro1a'
    # data_file_txt = 'coldStart vA20230305 BB.txt'; unit_key = 'pro1a'; cutback_gain_sclr_in = 0.5; ds_voc_soc_in = 0.06
    # data_file_txt = 'dwell noise Ca.5 v20221028.txt'  # ; dTb = [[0., 18000.],  [0, 8.]]
    # data_file_txt = 'dwell Ca.5 v20221028.txt'  # ; time_end_in=0.5  # ; dTb = [[0., 18000.],  [0, 8.]]

    # The following CHINS runs were sources of figures in the .odt report.  Chem is in data stream
    # in the following line I forgot to nominalize sp on load so scale_hys was 1.5 by mistake for baseline run.
    # data_file_txt = 'sat v20230128 20230201.txt'; unit_key = 'soc0p';  scale_in = 1.127; s_hys_in = 1.5; s_hys_in = 1.15; #stauct_in=0.1; s_hys_cap_in=1.;
    # data_file_txt = 'steps v20230128 20230203.txt'; unit_key = 'soc0p';  scale_in = 1.127; sres0_in = 1; sresct_in = 1; stauct_in = 1; s_hys_chg_in = 1; s_hys_dis_in = 1; s_cap_chg_in = 1.; s_cap_dis_in = 1.; tune_in = True # 0.9 tune 4, 5
    # data_file_txt = 'steps v20230128 20230204.txt'; unit_key = 'soc0p';  scale_in = 1.127; sres0_in = 1; sresct_in = 1; stauct_in = 1; s_hys_chg_in = 1; s_hys_dis_in = 1; s_cap_chg_in = 1.; s_cap_dis_in = 1.; tune_in = True  # 0.8 tune 4, 5 set s_hys_chg/dis = 0 to see prediction for R
    # data_file_txt = 'steps v20230128 20230214.txt'; unit_key = 'soc0p';  scale_in = 1.127; sres0_in = 1; sresct_in = 1; stauct_in = 1; s_hys_chg_in = 1; s_hys_dis_in = 1; s_cap_chg_in = 1.; s_cap_dis_in = 1.; tune_in = True  # ; time_end_in = 450  # 0.4 tune 4, 5 set s_hys_chg/dis = 0 to see prediction for R
    # data_file_txt = 'steps v20230128 20230218.txt'; unit_key = 'soc0p';  scale_in = 1.127; sres0_in = 1; sresct_in = 1; stauct_in = 1; s_hys_chg_in = 1; s_hys_dis_in = 1; s_cap_chg_in = 1.; s_cap_dis_in = 1.; tune_in = True  # ; time_end_in = 450  # 0.1 tune 4, 5 set s_hys_chg/dis = 0 to see prediction for R
    # data_file_txt = 'dw_pro1a_bb.csv'; unit_key = 'pro1a'; dvoc_mon_in = 0.2

    # Repeat of CHINS steps but for BB in truck
    # data_file_txt = 'steps vA20230305 20230324 BB.txt'; unit_key = 'soc1a'  ; tune_in = True; dvoc_mon_in = 0.16; dvoc_sim_in = 0.16;
    # temp_file = 'steps vA20230326 20230328.txt'; cat(temp_file, ('puttyLog20230328_113851.csv', 'puttyLog20230328_131552.csv'), in_path=path_to_data, out_path=rel_path_to_temp); unit_key = 'soc1a'; tune_in = True; dvoc_mon_in = 0.11; dvoc_sim_in = dvoc_mon_in; sres0_in = 2.5; sresct_in = 0.13; stauct_in = 1.;
    # temp_file = 'steps vA20230326 20230328.txt'; cat(temp_file, ('puttyLog20230328_113851.csv', 'puttyLog20230328_131552.csv'), in_path=path_to_data, out_path=rel_path_to_temp); unit_key = 'soc1a'; tune_in = True; dvoc_mon_in = 0.11; dvoc_sim_in = dvoc_mon_in;
    # data_file_txt = 'puttyLog20230328_131552_short.txt'; unit_key = 'soc1a'; tune_in = True; dvoc_mon_in = 0.11; dvoc_sim_in = dvoc_mon_in; sres0_in = 2.5; sresct_in = 0.13; stauct_in = 1;
    # data_file_txt = 'real world Xp20 30C 20220914.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_vb_raw = False; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; s_hys_in = 3.33; dvoc_mon_in = -0.05; dvoc_sim_in = -0.05
    # data_file_txt = 'real world Xp20 30C 20220914a+b.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_vb_raw = False; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; s_hys_in = 3.33; dvoc_mon_in = -0.05; dvoc_sim_in = -0.05
    # data_file_txt = 'real world Xp20 30C 20220917.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; init_time_in = -11110
    # data_file_txt = 'real world Xp20 v20220917a.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; init_time_in = -69900

    data_file_full = 'G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20231111b/rapidTweakRegression_pro3p2_bb.csv'
    unit_key_full = 'pro3p2_bb'
    compare_run_sim(data_file=data_file_full, unit_key=unit_key_full)
    # compare_run_sim()
