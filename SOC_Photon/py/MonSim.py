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

import numpy as np
from numpy.random import randn
import Battery
from Battery import Battery, BatteryMonitor, BatterySim, is_sat, Retained
from Battery import overall_batt
from TFDelay import TFDelay
from MonSimNomConfig import *  # Global config parameters.   Overwrite in your own calls for studies
from datetime import datetime, timedelta
from Scale import Scale


def save_clean_file(mon_ver, csv_file, unit_key):
    default_header_str = "unit,               hm,                  cTime,        dt,       sat,sel,mod,\
      Tb,  Vb,  Ib,  ioc,  voc_soc,    Vsat,dV_dyn,Voc_stat,Voc_ekf,     y_ekf,    soc_s,soc_ekf,soc,"
    n = len(mon_ver.time)
    date_time_start = datetime.now()
    with open(csv_file, "w") as output:
        output.write(default_header_str + "\n")
        for i in range(n):
            s = unit_key + ','
            dt_dt = timedelta(seconds=mon_ver.time[i]-mon_ver.time[0])
            time_stamp = date_time_start + dt_dt
            s += time_stamp.strftime("%Y-%m-%dT%H:%M:%S,")
            s += "{:7.3f},".format(mon_ver.time[i])
            s += "{:7.3f},".format(mon_ver.dt[i])
            s += "{:1.0f},".format(mon_ver.sat[i])
            s += "{:1.0f},".format(mon_ver.sel[i])
            s += "{:1.0f},".format(mon_ver.mod_data[i])
            s += "{:7.3f},".format(mon_ver.Tb[i])
            s += "{:7.3f},".format(mon_ver.Vb[i])
            s += "{:7.3f},".format(mon_ver.Ib[i])
            s += "{:7.3f},".format(mon_ver.ioc[i])
            s += "{:7.3f},".format(mon_ver.voc_soc[i])
            s += "{:7.3f},".format(mon_ver.Vsat[i])
            s += "{:7.3f},".format(mon_ver.dV_dyn[i])
            s += "{:7.3f},".format(mon_ver.Voc_stat[i])
            s += "{:7.3f},".format(mon_ver.Voc_ekf[i])
            s += "{:7.3f},".format(mon_ver.y_ekf[i])
            s += "{:7.3f},".format(mon_ver.soc_s[i])
            s += "{:7.3f},".format(mon_ver.soc_ekf[i])
            s += "{:7.3f},".format(mon_ver.soc[i])
            s += "\n"
            output.write(s)
        print("Wrote(save_clean_file):", csv_file)


def save_clean_file_sim(sim_ver, csv_file, unit_key):
    header_str = "unit_m,c_time,Tb_s,Tbl_s,vsat_s,voc_stat_s,dv_dyn_s,vb_s,ib_s,sat_s,dq_s,\
    soc_s,reset_s,"
    n = len(sim_ver.time)
    with open(csv_file, "w") as output:
        output.write(header_str + "\n")
        for i in range(n):
            s = unit_key + ','
            s += "{:13.3f},".format(sim_ver.time[i])
            s += "{:5.2f},".format(sim_ver.Tb_s[i])
            s += "{:5.2f},".format(sim_ver.Tbl_s[i])
            s += "{:8.3f},".format(sim_ver.vsat_s[i])
            s += "{:5.2f},".format(sim_ver.voc_stat_s[i])
            s += "{:5.2f},".format(sim_ver.dv_dyn_s[i])
            s += "{:5.2f},".format(sim_ver.vb_s[i])
            s += "{:8.3f},".format(sim_ver.ib_s[i])
            s += "{:7.3f},".format(sim_ver.sat_s[i])
            s += "{:5.3f},".format(sim_ver.dq_s[i])
            s += "{:7.3f},".format(sim_ver.soc_s[i])
            s += "{:7.3f},".format(sim_ver.reset_s[i])
            s += "\n"
            output.write(s)
        print("Wrote(save_clean_file_sim):", csv_file)


def replicate(mon_old, sim_old=None, init_time=-4., dv_hys=0., sres=1., t_Vb_fail=None, Vb_fail=13.2,
              t_Ib_fail=None, Ib_fail=0., use_ib_mon=False, scale_in=None, Bsim=None, Bmon=None, use_Vb_raw=False,
              scale_r_ss=1., s_hys_sim=1., s_hys_mon=1., dvsoc_sim=0., dvsoc_mon=0.):
    if sim_old and len(sim_old.time) < len(mon_old.time):
        t = sim_old.time
    else:
        t = mon_old.time
    reset_sel = mon_old.res
    if use_Vb_raw:
        Vb = mon_old.Vb_h
    else:
        Vb = mon_old.Vb
    Ib_past = mon_old.Ib_past
    Tb = mon_old.Tb
    soc_init = mon_old.soc[0]
    soc_ekf_init = mon_old.soc_ekf[0]
    soc_s_init = mon_old.soc_s[0]
    sat_init = mon_old.sat[0]
    chm_m = mon_old.chm
    chm_s = sim_old.chm_s
    if sim_old:
        sat_s_init = sim_old.sat_s[0]
    else:
        sat_s_init = mon_old.Voc_stat[0] > mon_old.Vsat[0]
    t_len = len(t)
    rp = Retained()
    rp.modeling = mon_old.mod()
    print("rp.modeling is ", rp.modeling)
    tweak_test = rp.tweak_test()
    temp_c = mon_old.Tb[0]

    # Setup
    scale = model_bat_cap / Battery.RATED_BATT_CAP
    if scale_in:
        scale *= scale_in
    s_q = Scale(1., 3., 0.000005, 0.00005)
    s_r = Scale(1., 3., 0.001, 1.)   # t_Ib_fail = 1000 o
    sim = BatterySim(temp_c=temp_c, tau_ct=tau_ct, scale=scale, hys_scale=hys_scale, tweak_test=tweak_test,
                     dv_hys=dv_hys, sres=sres, scale_r_ss=scale_r_ss, s_hys=s_hys_sim, dvoc=dvsoc_sim)
    mon = BatteryMonitor(r_sd=rsd, tau_sd=tau_sd, r0=r0, tau_ct=tau_ct, r_ct=rct, tau_dif=tau_dif, r_dif=r_dif,
                         temp_c=temp_c, scale=scale, hys_scale=hys_scale_monitor, tweak_test=tweak_test, dv_hys=dv_hys, sres=sres,
                         scaler_q=s_q, scaler_r=s_r, scale_r_ss=scale_r_ss, s_hys=s_hys_mon, dvoc=dvsoc_mon)
    # need Tb input.   perhaps need higher order to enforce basic type 1 response
    Is_sat_delay = TFDelay(in_=mon_old.soc[0] > 0.97, t_true=T_SAT, t_false=T_DESAT, dt=0.1)  # later, dt is changed

    # time loop
    now = t[0]
    for i in range(t_len):
        now = t[i]
        reset = (t[i] <= init_time) or (t[i] < 0. and t[0] > init_time)
        if reset_sel is not None:
            reset = reset or reset_sel[i]
        mon_old.i = i
        if i == 0:
            T = t[1] - t[0]
        else:
            T = t[i] - t[i - 1]

        # dc_dc_on = bool(lut_dc.interp(t[i]))
        dc_dc_on = False

        if reset:
            sim.apply_soc(soc_s_init, Tb[i])
            rp.delta_q_model = sim.delta_q
            rp.t_last_model = Tb[i] + dt_model
            sim.load(rp.delta_q_model, rp.t_last_model)
            sim.init_battery()
            sim.apply_delta_q_t(rp.delta_q_model, rp.t_last_model)
            mon.sat = sat_init

        # Models
        if sim_old and not use_ib_mon:
            ib_in_s = sim_old.ib_in_s[i]
        else:
            ib_in_s = Ib_past[i]
        if Bsim is None:
            _chm_s = chm_s[i]
        else:
            _chm_s = Bsim
        sim.calculate(chem=_chm_s, temp_c=Tb[i], soc=sim.soc, curr_in=ib_in_s, dt=T, q_capacity=sim.q_capacity,
                      dc_dc_on=dc_dc_on, reset=reset, rp=rp, sat_init=sat_s_init)
        charge_curr = sim.ib
        sim.count_coulombs(chem=_chm_s, dt=T, reset=reset, temp_c=Tb[i], charge_curr=charge_curr, sat=False, soc_s_init=soc_s_init,
                           mon_sat=mon.sat, mon_delta_q=mon.delta_q)

        # EKF
        if reset:
            mon.apply_soc(soc_init, Tb[i])
            rp.delta_q = mon.delta_q
            rp.t_last = Tb[i]
            mon.load(rp.delta_q, rp.t_last)
            mon.assign_temp_c(Tb[i])
            mon.init_battery()
            mon.init_soc_ekf(soc_ekf_init)  # when modeling (assumed in python) ekf wants to equal model

        # Monitor calculations including ekf
        Tb_ = Tb[i]
        if Bmon is None:
            _chm_m = chm_m[i]
        else:
            _chm_m = Bmon
        if t_Ib_fail and t[i] > t_Ib_fail:
            Ib_ = Ib_fail
        else:
            Ib_ = mon_old.Ib[i]
        if t_Vb_fail and t[i] >= t_Vb_fail:
            Vb_ = Vb_fail
        else:
            Vb_ = Vb[i]
        if rp.modeling == 0:
            mon.calculate(_chm_m, Tb_, Vb_, Ib_, T, rp=rp, reset=reset)
        else:
            mon.calculate(_chm_m, Tb_, Vb_ + randn() * v_std + dv_sense, Ib_ + randn() * i_std + di_sense, T, rp=rp,
                          reset=reset, d_voc=None)
        sat = is_sat(Tb[i], mon.voc, mon.soc)
        saturated = Is_sat_delay.calculate(sat, T_SAT, T_DESAT, min(T, T_SAT / 2.), reset)
        if rp.modeling == 0:
            mon.count_coulombs(chem=_chm_m, dt=T, reset=reset, temp_c=Tb[i], charge_curr=Ib_, sat=saturated)
        else:
            mon.count_coulombs(chem=_chm_m, dt=T, reset=reset, temp_c=temp_c, charge_curr=Ib_, sat=saturated)
        mon.calc_charge_time(mon.q, mon.q_capacity, charge_curr, mon.soc)
        # mon.regauge(Tb[i])
        mon.assign_soc_s(sim.soc)

        # Plot stuff
        mon.save(t[i], T, mon.soc, sim.voc)
        sim.save(t[i], T)
        sim.save_s(t[i])

        # Print initial
        if i == 0:
            print('time=', t[i])
            print('mon:  ', str(mon))
            print('time=', t[i])
            print('sim:  ', str(sim))
        # if t[i]>29495:
        #     print('time=', t[i])
        #     print('mon:  ', str(mon))
        #     print('time=', t[i])
        #     print('sim:  ', str(sim))
        #     print(t[i])

    # Data
    print('time=', now)
    print('mon:  ', str(mon))
    print('sim:  ', str(sim))

    return mon.saved, sim.saved, mon.Randles.saved, sim.saved_s


if __name__ == '__main__':
    import sys
    from DataOverModel import SavedData, SavedDataSim, write_clean_file, overall
    from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
    import matplotlib.pyplot as plt
    plt.rcParams['axes.grid'] = True

    def main():
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        date_ = datetime.now().strftime("%y%m%d")

        # Transient  inputs
        # time_end = None
        time_end = 35200
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
        dvsoc_sim_in = 0.
        dvsoc_mon_in = 0.
        Bmon_in = None
        Bsim_in = None
        skip = 1
        # Save these
        # data_file_old_txt = '../dataReduction/real world Xp20 20220902.txt'; unit_key = 'soc0_2022'; use_ib_mon_in=True; scale_in=1.12

        # Regression suite
        # data_file_old_txt = '../dataReduction/ampHiFail20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = '../dataReduction/ampLoFail20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = '../dataReduction/ampHiFailNoise20220914.txt'; unit_key = 'pro_2022';
        # data_file_old_txt = '../dataReduction/rapidTweakRegression20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = '../dataReduction/rapidTweakRegression40C_20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = '../dataReduction/slowTweakRegression20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = '../dataReduction/triTweakDisch20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = '../dataReduction/satSit20220914.txt'; unit_key = 'pro_2022';
        # data_file_old_txt = '../dataReduction/ampHiFailSlow20220914.txt'; unit_key = 'pro_2022';
        # data_file_old_txt = '../dataReduction/vHiFail20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = '../dataReduction/pulse20220914.txt'; unit_key = 'pro_2022'; init_time_in=-0.001;
        # data_file_old_txt = '../dataReduction/tbFailMod20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = '../dataReduction/tbFailHdwe20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = '../dataReduction/real world Xp20 20220907.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_Vb_raw = True; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; scale_hys_sim_in = 3.33; Bsim_in = 0; skip=4
        # data_file_old_txt = '../dataReduction/real world Xp20 20220910.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_Vb_raw = True; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; scale_hys_sim_in = 3.33;
        data_file_old_txt = '../dataReduction/real world Xp20 30C 20220914.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_Vb_raw = False; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; scale_hys_sim_in = 3.33; dvsoc_mon_in = -0.05; dvsoc_sim_in = -0.05
        # data_file_old_txt = '../dataReduction/real world Xp20 30C 20220914a+b.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_Vb_raw = False; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; scale_hys_sim_in = 3.33; dvsoc_mon_in = 0.05
        title_key = "unit,"  # Find one instance of title
        title_key_sel = "unit_s,"  # Find one instance of title
        unit_key_sel = "unit_sel"
        title_key_sim = "unit_m,"  # Find one instance of title
        unit_key_sim = "unit_sim"

        # Load mon v4 (old)
        data_file_clean = write_clean_file(data_file_old_txt, type_='_mon', title_key=title_key, unit_key=unit_key, skip=skip)
        cols = ('cTime', 'dt', 'chm', 'sat', 'sel', 'mod', 'Tb', 'Vb', 'Ib', 'ioc', 'voc_soc', 'Vsat', 'dV_dyn', 'Voc_stat',
                'Voc_ekf', 'y_ekf', 'soc_s', 'soc_ekf', 'soc')
        mon_old_raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols,  dtype=float,
                                    encoding=None).view(np.recarray)

        # Load sel (old)
        sel_file_clean = write_clean_file(data_file_old_txt, type_='_sel', title_key=title_key_sel,
                                          unit_key=unit_key_sel, skip=skip)
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
        mon_old = SavedData(data=mon_old_raw, sel=sel_old_raw, time_end=time_end)

        # Load _m v24 portion of real-time run (old)
        data_file_sim_clean = write_clean_file(data_file_old_txt, type_='_sim', title_key=title_key_sim,
                                               unit_key=unit_key_sim, skip=skip)
        cols_sim = ('c_time', 'chm_s', 'Tb_s', 'Tbl_s', 'vsat_s', 'voc_stat_s', 'dv_dyn_s', 'vb_s', 'ib_s',
                    'ib_in_s', 'ioc_s', 'sat_s', 'dq_s', 'soc_s', 'reset_s')
        if data_file_sim_clean:
            sim_old_raw = np.genfromtxt(data_file_sim_clean, delimiter=',', names=True, usecols=cols_sim,
                                        dtype=float, encoding=None).view(np.recarray)
            sim_old = SavedDataSim(time_ref=mon_old.time_ref, data=sim_old_raw, time_end=time_end)
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
                                                             dvsoc_sim=dvsoc_sim_in, dvsoc_mon=dvsoc_mon_in,
                                                             Bmon=Bmon_in, Bsim=Bsim_in)
        save_clean_file(mon_ver, mon_file_save, 'mon_rep' + date_)

        # Plots
        n_fig = 0
        fig_files = []
        data_root = data_file_clean.split('/')[-1].replace('.csv', '-')
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        n_fig, fig_files = overall_batt(mon_ver, sim_ver, randles_ver, filename, fig_files, plot_title=plot_title,
                                        n_fig=n_fig, suffix='_ver')  # sim over mon verify
        n_fig, fig_files = overall(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                   plot_title=plot_title, n_fig=n_fig)  # all over all
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='../dataReduction/figures')
        cleanup_fig_files(fig_files)

        plt.show()

    main()
