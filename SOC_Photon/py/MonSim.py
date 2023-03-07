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
from numpy.random import randn
import Battery
from Battery import Battery, BatteryMonitor, BatterySim, is_sat, Retained
from Battery import overall_batt
from TFDelay import TFDelay
from MonSimNomConfig import *  # Global config parameters.   Overwrite in your own calls for studies
from datetime import datetime, timedelta
from Scale import Scale
from pyDAGx import myTables
import statistics as sts



def save_clean_file(mon_ver, csv_file, unit_key):
    default_header_str = "unit,               hm,                  cTime,        dt,       sat,sel,mod,\
      Tb,  vb,  ib,  ioc,  voc_soc,    vsat,dv_dyn,voc_stat,voc_ekf,     y_ekf,    soc_s,soc_ekf,soc,"
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
            s += "{:7.3f},".format(mon_ver.vb[i])
            s += "{:7.3f},".format(mon_ver.ib[i])
            s += "{:7.3f},".format(mon_ver.ioc[i])
            s += "{:7.3f},".format(mon_ver.voc_soc[i])
            s += "{:7.3f},".format(mon_ver.vsat[i])
            s += "{:7.3f},".format(mon_ver.dv_dyn[i])
            s += "{:7.3f},".format(mon_ver.voc_stat[i])
            s += "{:7.3f},".format(mon_ver.voc_ekf[i])
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


#  Replicate the application in its entirety here.
#  There are no 'bank' parameters anywhere in this model.   It is assumed that all inputs from the application have
#  been converted to the single battery unit 12v form, S1P1, lower-case nomenclature.
def replicate(mon_old, sim_old=None, init_time=-4., t_vb_fail=None, vb_fail=13.2,
              t_ib_fail=None, ib_fail=0., use_ib_mon=False, scale_in=None, Bsim=None, Bmon=None, use_vb_raw=False,
              scale_r_ss=1., s_hys_sim=1., s_hys_mon=1., dvoc_sim=0., dvoc_mon=0., drive_ekf=False, dTb_in=None,
              verbose=True, t_max=None, eframe_mult=Battery.cp_eframe_mult, sres0=1., sresct=1., stauct_sim=1.,
              stauct_mon=1, use_vb_sim=False, scale_hys_cap_sim=1., scale_hys_cap_mon=1., s_cap_chg=1., s_cap_dis=1.,
              s_hys_chg=1., s_hys_dis=1., s_coul_eff=1., use_mon_soc=False, cutback_gain_sclr=1., ds_voc_soc=0.):
    if sim_old is not None and len(sim_old.time) < len(mon_old.time):
        t = sim_old.time
    else:
        t = mon_old.time
    updateTimeIn = sts.mode(mon_old.dt)
    if t_max is not None:
        t_delt = t - t[0]
        t = t[np.where(t_delt <= t_max)]
    reset_sel = mon_old.res
    if use_vb_raw:
        vb = mon_old.vb_h
    else:
        vb = mon_old.vb
    # ib_past = mon_old.ib_past
    ib_in = mon_old.ib
    Tb = mon_old.Tb
    soc_s_init = mon_old.soc_s[0]
    sat_init = mon_old.sat[0]
    dv_hys_init = mon_old.dv_hys[0]
    chm_m = mon_old.chm
    chm_s = sim_old.chm_s
    if sim_old is not None:
        sat_s_init = sim_old.sat_s[0]
    else:
        sat_s_init = mon_old.voc_stat[0] > mon_old.vsat[0]
    t_len = len(t)
    rp = Retained()
    try:
        rp.modeling = mon_old.mod()
    except:
        rp.modeling = mon_old.mod_data[0]
    print("rp.modeling is ", rp.modeling)
    print('use_ib_mon is', use_ib_mon)
    tweak_test = rp.tweak_test()
    temp_c = mon_old.Tb[0]
    if dTb_in is not None:
        dTb_in = np.array(dTb_in)
        temp_c += dTb_in[1, 0]
        lut_dTb = myTables.TableInterp1D( np.array(dTb_in[0, :]), np.array(dTb_in[1, :]))

    # Setup
    if hasattr(mon_old, 'qcrs'):
        scale_mon = mon_old.qcrs[0] / (Battery.UNIT_CAP_RATED*3600)
    else:
        scale_mon = unit_cap_rated / Battery.UNIT_CAP_RATED
        if scale_in:
            scale_mon *= scale_in
    if sim_old is not None and hasattr(sim_old, 'qcrs_s'):
        scale_sim = sim_old.qcrs_s[0] / (Battery.UNIT_CAP_RATED*3600)
    else:
        scale_sim = unit_cap_rated / Battery.UNIT_CAP_RATED
        if scale_in:
            scale_sim *= scale_in
    s_q = Scale(1., 3., 0.000005, 0.00005)
    s_r = Scale(1., 3., 0.001, 1.)   # t_ib_fail = 1000
    sim = BatterySim(mod_code=chm_s[0], temp_c=temp_c, scale=scale_sim, tweak_test=tweak_test,
                     dv_hys=dv_hys_init, sres0=sres0, sresct=sresct, stauct=stauct_sim, scale_r_ss=scale_r_ss,
                     s_hys=s_hys_sim, dvoc=dvoc_sim, scale_hys_cap=scale_hys_cap_sim, s_coul_eff=s_coul_eff,
                     s_cap_chg=s_cap_chg, s_cap_dis=s_cap_dis, s_hys_chg=s_hys_chg, s_hys_dis=s_hys_dis,
                     cutback_gain_sclr=cutback_gain_sclr, ds_voc_soc=ds_voc_soc)
    mon = BatteryMonitor(mod_code=chm_m[0], temp_c=temp_c, scale=scale_mon, tweak_test=tweak_test, dv_hys=dv_hys_init,
                         sres0=sres0, sresct=sresct, stauct=stauct_mon, scaler_q=s_q, scaler_r=s_r,
                         scale_r_ss=scale_r_ss, s_hys=s_hys_mon, dvoc=dvoc_mon, eframe_mult=eframe_mult,
                         scale_hys_cap=scale_hys_cap_mon, s_coul_eff=s_coul_eff, s_cap_chg=s_cap_chg,
                         s_cap_dis=s_cap_dis, s_hys_chg=s_hys_chg, s_hys_dis=s_hys_dis)
    # need Tb input.   perhaps need higher order to enforce basic type 1 response
    Is_sat_delay = TFDelay(in_=mon_old.soc[0] > 0.97, t_true=T_SAT, t_false=T_DESAT, dt=0.1)  # later, dt is changed
    bms_off_init = mon_old.bms_off[0]

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
        if dTb_in is not None:
            dTb = lut_dTb.interp(t[i])
        else:
            dTb = 0.
        # dc_dc_on = bool(lut_dc.interp(t[i]))
        dc_dc_on = False
        Tb_ = Tb[i]+dTb

        # Basic reset model verification is to init to the input data
        # Tried hard not to re-implement solvers in the Python verification  tool
        # Also, BTW, did not implement signal selection or tweak logic
        if reset:
            sim.apply_soc(mon_old.soc_s[i], Tb_)
            rp.delta_q_model = sim.delta_q
            rp.t_last_model = Tb_
            sim.load(rp.delta_q_model, rp.t_last_model)
            sim.apply_delta_q_t(rp.delta_q_model, rp.t_last_model)
            if sim_old is not None:
                sat_s_init = sim_old.sat_s[0]
            else:
                sat_s_init = mon_old.voc_stat[0] > mon_old.vsat[0]
            sim.sat = sat_s_init
            mon.sat = sat_init
            mon.hys.dv_hys = mon_old.dv_hys[i]

        # Models
        if sim_old is not None and not use_ib_mon:
            ib_in_s = sim_old.ib_in_s[i]
        else:
            ib_in_s = ib_in[i]
        if Bsim is None:
            _chm_s = chm_s[i]
        else:
            _chm_s = Bsim
        sim.calculate(chem=_chm_s, temp_c=Tb_, soc=sim.soc, curr_in=ib_in_s, dt=T, q_capacity=sim.q_capacity,
                      dc_dc_on=dc_dc_on, reset=reset, updateTimeIn=updateTimeIn, rp=rp, sat_init=sat_s_init,
                      bms_off_init=bms_off_init)
        sim.count_coulombs(chem=_chm_s, dt=T, reset=reset, temp_c=Tb_, charge_curr=sim.ib_charge, sat=False, soc_s_init=soc_s_init,
                           mon_sat=mon.sat, mon_delta_q=mon.delta_q, use_soc_in=use_mon_soc, soc_in=mon_old.soc[i])

        # EKF
        if reset:
            mon.apply_soc(mon_old.soc[i], Tb_)
            rp.delta_q = mon.delta_q
            rp.t_last = Tb_
            mon.load(rp.delta_q, rp.t_last)
            mon.assign_temp_c(Tb_)
            mon.init_soc_ekf(mon_old.soc_ekf[i])  # when modeling (assumed in python) ekf wants to equal model

        # Monitor calculations including ekf
        if Bmon is None:
            _chm_m = chm_m[i]
        else:
            _chm_m = Bmon
        if t_ib_fail and t[i] > t_ib_fail:
            ib_ = ib_fail
        else:
            if mon_old.ib_sel is not None:
                ib_ = mon_old.ib_sel[i]
            else:
                ib_ = mon_old.ib[i]
                # ib_ = sim.ib
        if use_vb_sim:
            vb_ = sim.vb
        elif t_vb_fail and t[i] >= t_vb_fail:
            vb_ = vb_fail
        else:
            vb_ = vb[i]
        if drive_ekf:
            u_old = mon_old.u[i]
            z_old = mon_old.z[i]
            x_old = mon_old.x[i]
        else:
            u_old = None
            z_old = None
            x_old = None
        if rp.modeling == 0:
            mon.calculate(_chm_m, Tb_, vb_, ib_, T, rp=rp, reset=reset, updateTimeIn=updateTimeIn, u_old=u_old,
                          z_old=z_old, x_old=x_old, bms_off_init=bms_off_init)
        else:
            mon.calculate(_chm_m, Tb_, vb_ + randn() * v_std + dv_sense, ib_ + randn() * i_std + di_sense, T, rp=rp,
                          reset=reset, updateTimeIn=updateTimeIn, u_old=u_old, z_old=z_old, x_old=x_old,
                          bms_off_init=bms_off_init)
        ib_charge = mon.ib_charge
        sat = is_sat(Tb_, mon.voc_filt, mon.soc, mon.chemistry.nom_vsat, mon.chemistry.dvoc_dt, mon.chemistry.low_t)
        saturated = Is_sat_delay.calculate(sat, T_SAT, T_DESAT, min(T, T_SAT / 2.), reset)
        if rp.modeling == 0:
            mon.count_coulombs(chem=_chm_m, dt=T, reset=reset, temp_c=Tb_, charge_curr=ib_charge, sat=saturated,
                               use_soc_in=use_mon_soc, soc_in=mon_old.soc[i])
        else:
            mon.count_coulombs(chem=_chm_m, dt=T, reset=reset, temp_c=Tb_, charge_curr=ib_charge, sat=saturated,
                               use_soc_in=use_mon_soc, soc_in=mon_old.soc[i])
        mon.calc_charge_time(mon.q, mon.q_capacity, ib_charge, mon.soc)
        mon.assign_soc_s(sim.soc)
        # Plot stuff
        mon.save(t[i], T, mon.soc, sim.voc)
        sim.save(t[i], T)
        sim.save_s(t[i])

        # Print initial
        if i == 0 and verbose:
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
        if verbose:
            print("{:9.3f}".format(t[i]), "{:4.0f}".format(mon_old.chm[i]), "{:4.0f}".format(sim_old.chm_s[i]),
                  "{:9.3f}".format(sim_old.ib_in_s[i]), "{:9.3f}".format(sim_old.dv_hys_s[i]),
                  "{:9.3f}".format(mon_old.ib[i]), "{:12.7f}".format(mon_old.soc[i]), "{:9.3f}".format(mon_old.dv_hys[i]),
                  "{:9.3f}".format(sim.saved_s.ib_in_s[i]), "{:9.3f}".format(sim.hys.ibs), "{:9.3f}".format(sim.hys.ioc),
                  "{:4.0f}".format(sim.sat), "{:9.3f}".format(sim.hys.disabled), "{:9.3f}".format(sim.hys.dv_dot),
                  "{:9.3f}".format(sim.saved.dv_hys[i]), "{:9.3f}".format(mon.saved.ib[i]), "{:12.7f}".format(mon.saved.soc[i]),
                  "{:9.3f}".format(mon.hys.ibs), "{:9.3f}".format(mon.hys.ioc), "{:4.0f}".format(mon.sat),
                  "{:9.3f}".format(mon.hys.disabled), "{:9.3f}".format(mon.hys.dv_dot),
                  "{:9.3f}".format(mon.saved.dv_hys[i]))

    # Data
    if verbose:
        print('   time mo.chm so.chm so.ib_in_s so.dv_hys  mo.ib mo.soc mo.dv_hys   smv.ib_in_s sim.ibs sim.ioc sim.sat sim.dis sim.dv_dot smv.dv_hys  mv.ib  mv.soc mon.ibs  mon.ioc   mon.sat   mon.dis    mon.dv_dot  mv.dv_hys')
        print('time=', now)
        print('mon:  ', str(mon))
        print('sim:  ', str(sim))

    return mon.saved, sim.saved, sim.saved_s, mon, sim


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
        time_end = None
        # time_end = 35200
        t_ib_fail = None
        init_time_in = None
        use_ib_mon_in = False
        scale_in = None
        use_vb_raw = False
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
        # Save these
        # data_file_old_txt = '../dataReduction/real world Xp20 20220902.txt'; unit_key = 'soc0_2022'; use_ib_mon_in=True; scale_in=1.12

        # Regression suite
        # data_file_old_txt = 'ampHiFail20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'ampLoFail20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'ampHiFailNoise20220914.txt'; unit_key = 'pro_2022';
        # data_file_old_txt = 'rapidTweakRegression20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'rapidTweakRegression40C_20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'slowTweakRegression20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'triTweakDisch20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'satSit20220914.txt'; unit_key = 'pro_2022';
        # data_file_old_txt = 'ampHiFailSlow20220914.txt'; unit_key = 'pro_2022';
        # data_file_old_txt = 'vHiFail20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'pulse20220914.txt'; unit_key = 'pro_2022'; init_time_in=-0.001;
        # data_file_old_txt = 'tbFailMod20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'tbFailHdwe20220914.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'real world Xp20 30C 20220914.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_vb_raw = False; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; scale_hys_sim_in = 3.33; dvoc_mon_in = -0.05; dvoc_sim_in = -0.05
        # data_file_old_txt = 'real world Xp20 30C 20220914a+b.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; use_vb_raw = False; scale_r_ss_in = 1.; scale_hys_mon_in = 3.33; scale_hys_sim_in = 3.33; dvoc_mon_in = -0.05; dvoc_sim_in = -0.05
        # data_file_old_txt = 'real world Xp20 30C 20220917.txt'; unit_key = 'soc0_2022'; scale_in = 1.084; init_time_in = -11110
        # data_file_old_txt = 'EKF_Track 20200917.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'EKF_Track Dr100 v20220917.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'EKF_Track Dr200 v20220917.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'EKF_Track Dr400 v20220917.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'EKF_Track Dr800 v20220917.txt'; unit_key = 'pro_2022'
        data_file_old_txt = 'EKF_Track Dr2000 v20220917.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'EKF_Track Dr200 Xf0p04 v20220917.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'EKF_Track Dr400 Xf0p04 v20220917.txt'; unit_key = 'pro_2022'
        # data_file_old_txt = 'EKF_Track Dr800 Xf0p04 v20220917.txt'; unit_key = 'pro_2022'
        title_key = "unit,"  # Find one instance of title
        title_key_sel = "unit_s,"  # Find one instance of title
        unit_key_sel = "unit_sel"
        title_key_sim = "unit_m,"  # Find one instance of title
        unit_key_sim = "unit_sim"
        pathToSavePdfTo = '../dataReduction/figures'
        path_to_data = '../dataReduction'
        path_to_temp = '../dataReduction/temp'

        # Load mon v4 (old)
        data_file_clean = write_clean_file(data_file_old_txt, type_='_mon', title_key=title_key, unit_key=unit_key,
                                           skip=skip, path_to_data=path_to_data, path_to_temp=path_to_temp)
        cols = ('cTime', 'dt', 'chm', 'sat', 'sel', 'mod', 'Tb', 'vb', 'ib', 'ioc', 'voc_soc', 'vsat', 'dv_dyn', 'voc_stat',
                'voc_ekf', 'y_ekf', 'soc_s', 'soc_ekf', 'soc')
        mon_old_raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols,  dtype=float,
                                    encoding=None).view(np.recarray)

        # Load sel (old)
        sel_file_clean = write_clean_file(data_file_old_txt, type_='_sel', title_key=title_key_sel,
                                          unit_key=unit_key_sel, skip=skip, path_to_data=path_to_data,
                                          path_to_temp=path_to_temp)
        cols_sel = ('c_time', 'res', 'user_sel', 'cc_dif',
                    'ibmh', 'ibnh', 'ibmm', 'ibnm', 'ibm', 'ib_diff', 'ib_diff_f',
                    'voc_soc', 'e_wrap', 'e_wrap_filt',
                    'ib_sel_stat', 'ib_h', 'ib_s', 'mib', 'ib',
                    'vb_sel', 'vb_h', 'vb_s', 'mvb', 'vb',
                    'Tb_h', 'Tb_s', 'mtb', 'Tb_f',
                    'fltw', 'falw', 'ib_rate', 'ib_quiet', 'tb_sel',
                    'ccd_thr', 'ewh_thr', 'ewl_thr', 'ibd_thr', 'ibq_thr', 'preserving')
        sel_old_raw = None
        if sel_file_clean:
            sel_old_raw = np.genfromtxt(sel_file_clean, delimiter=',', names=True, usecols=cols_sel, dtype=float,
                                        encoding=None).view(np.recarray)
        mon_old = SavedData(data=mon_old_raw, sel=sel_old_raw, time_end=time_end)

        # Load _m v24 portion of real-time run (old)
        data_file_sim_clean = write_clean_file(data_file_old_txt, type_='_sim', title_key=title_key_sim,
                                               unit_key=unit_key_sim, skip=skip, path_to_data=path_to_data,
                                               path_to_temp=path_to_temp)
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

        # New run
        mon_file_save = data_file_clean.replace(".csv", "_rep.csv")
        mon_ver, sim_ver, sim_s_ver = replicate(mon_old, sim_old=sim_old, init_time=init_time,
                                                sres0=1.0, sresct=1.0, t_ib_fail=t_ib_fail,
                                                use_ib_mon=use_ib_mon_in, scale_in=scale_in,
                                                use_vb_raw=use_vb_raw, scale_r_ss=scale_r_ss_in,
                                                s_hys_sim=scale_hys_sim_in, s_hys_mon=scale_hys_mon_in,
                                                dvoc_sim=dvoc_sim_in, dvoc_mon=dvoc_mon_in,
                                                Bmon=Bmon_in, Bsim=Bsim_in)
        save_clean_file(mon_ver, mon_file_save, 'mon_rep' + date_)

        # Plots
        n_fig = 0
        fig_files = []
        data_root = data_file_clean.split('/')[-1].replace('.csv', '-')
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        n_fig, fig_files = overall_batt(mon_ver, sim_ver, filename, fig_files, plot_title=plot_title,
                                        n_fig=n_fig, suffix='_ver')  # sim over mon verify
        n_fig, fig_files = overall(mon_old, mon_ver, sim_old, sim_ver, sim_s_ver, filename, fig_files,
                                   plot_title=plot_title, n_fig=n_fig)  # all over all
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo=pathToSavePdfTo)
        cleanup_fig_files(fig_files)

        plt.show()

    main()
