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
from Battery import Battery, BatteryMonitor, BatteryModel, is_sat, Retained
from Battery import overall as overalls
from TFDelay import TFDelay
from MonSimNomConfig import *  # Global config parameters.   Overwrite in your own calls for studies
from datetime import datetime, timedelta


def save_clean_file(mons, csv_file, unit_key):
    default_header_str = "unit,               hm,                  cTime,        dt,       sat,sel,mod,  Tb,  Vb,  Ib,        Vsat,dV_dyn,Voc_stat,Voc_ekf,     y_ekf,    soc_m,soc_ekf,soc,soc_wt,"
    n = len(mons.time)
    date_time_start = datetime.now()
    with open(csv_file, "w") as output:
        output.write(default_header_str + "\n")
        for i in range(n):
            s = unit_key + ','
            dt_dt = timedelta(seconds=mons.time[i]-mons.time[0])
            time_stamp = date_time_start + dt_dt
            s += time_stamp.strftime("%Y-%m-%dT%H:%M:%S,")
            s += "{:7.3f},".format(mons.time[i])
            s += "{:7.3f},".format(mons.dt[i])
            s += "{:d},".format(mons.sat[i])
            s += "{:d},".format(mons.sel[i])
            s += "{:d},".format(mons.mod_data[i])
            s += "{:7.3f},".format(mons.Tb[i])
            s += "{:7.3f},".format(mons.Vb[i])
            s += "{:7.3f},".format(mons.Ib[i])
            s += "{:7.3f},".format(mons.Vsat[i])
            s += "{:7.3f},".format(mons.dV_dyn[i])
            s += "{:7.3f},".format(mons.Voc_stat[i])
            s += "{:7.3f},".format(mons.Voc_ekf[i])
            s += "{:7.3f},".format(mons.y_ekf[i])
            s += "{:7.3f},".format(mons.soc_m[i])
            s += "{:7.3f},".format(mons.soc_ekf[i])
            s += "{:7.3f},".format(mons.soc[i])
            s += "{:7.3f},".format(mons.soc_wt[i])
            s += "\n"
            output.write(s)
        print("Wrote(save_clean_file):", csv_file)

def save_clean_file_sim(sims, csv_file, unit_key):
    header_str = "unit_m,c_time,Tb_m,Tbl_m,vsat_m,voc_stat_m,dv_dyn_m,vb_m,ib_m,sat_m,ddq_m,dq_m,q_m,qcap_m,soc_m,reset_m,"
    n = len(sims.time)
    date_time_start = datetime.now()
    with open(csv_file, "w") as output:
        output.write(header_str + "\n")
        for i in range(n):
            s = unit_key + ','
            s += "{:13.3f},".format(sims.time[i])
            s += "{:5.2f},".format(sims.Tb_m[i])
            s += "{:5.2f},".format(sims.Tbl_m[i])
            s += "{:8.3f},".format(sims.vsat_m[i])
            s += "{:5.2f},".format(sims.voc_stat_m[i])
            s += "{:5.2f},".format(sims.dv_dyn_m[i])
            s += "{:5.2f},".format(sims.vb_m[i])
            s += "{:8.3f},".format(sims.ib_m[i])
            s += "{:7.3f},".format(sims.sat_m[i])
            s += "{:5.3f},".format(sims.ddq_m[i])
            s += "{:5.3f},".format(sims.dq_m[i])
            s += "{:5.3f},".format(sims.q_m[i])
            s += "{:5.3f},".format(sims.qcap_m[i])
            s += "{:7.3f},".format(sims.soc_m[i])
            s += "{:7.3f},".format(sims.reset_m[i])
            s += "\n"
            output.write(s)
        print("Wrote(save_clean_file_sim):", csv_file)

def replicate(saved_old, saved_sim_old=None, init_time=-4., dv_hys=0., sres=1., t_Vb_fail=None, Vb_fail=13.2, t_Ib_fail=None, Ib_fail=0.):
    t = saved_old.time
    dt = saved_old.dt
    Vb = saved_old.Vb
    Ib = saved_old.Ib
    Ib_past = saved_old.Ib_past
    Tb = saved_old.Tb
    Voc = saved_old.Voc
    Voc_stat = saved_old.Voc_stat
    soc_init = saved_old.soc[0]
    soc_m_init = saved_old.soc_m[0]
    t_len = len(t)
    rp = Retained()
    rp.modeling = saved_old.mod()
    print("rp.modeling is ", rp.modeling)
    tweak_test = rp.tweak_test()
    temp_c = saved_old.Tb[0]

    # Setup
    scale = model_bat_cap / Battery.RATED_BATT_CAP
    # q = 0.001; r = 0.1  # base 0.72 / 0.85 / +-0.05 base
    # q = 0.01; r = 0.01  # base 0.72 / 0.9 / +-0.07  noisy
    # q = 0.1; r = 0.001  # base 0.72 / 0.9 / +-0.07  noisier
    # q = 0.002; r = 0.1  # base 0.73 / 0.86 / +-0.055 noisy
    # q = 0.005; r = 0.1  # base 0.73 / 0.86 / +-0.055 noisy
    # q = 0.01; r = 0.1  # base 0.73 / 0.86 / +-0.055 noisy
    # q = 0.1; r = 0.1  # base 0.73 / 0.86 / +-0.055 noisier
    # q = 0.005; r = 0.05  # base 0.73 / 0.86 / +-0.055 noisy
    # q = 0.05; r = 0.05  # base 0.73 / 0.86 / +-0.055 noisier
    # q = 0.001; r = 0.05  # base 0.73 / 0.86 / +-0.055 like base
    # q = 0.001; r = 0.01  # base 0.73 / 0.86 / +-0.055 noisier
    # q = 0.001; r = 0.001  # base 0.73 / 0.86 / +-0.055 noisier
    # q = 0.0001; r = 0.05  # base 0.75 / 0.83 / +-0.04 good
    # q = 0.0001; r = 0.1  # base 0.74 / 0.8 / +-0.03 better
    # q = 0.00002; r = 0.2  # base 0.4 / n/a / +-0.0 falls apart
    # q = 0.0001; r = 0.2  # base 0.74 / 0.8 / +-0.03 best (temperature bias apparent. EKF doesn't account for it)
    # q = 0.0001; r = 0.5  # base 0.74 / 0.8 / +-0.03 even better than best
    # q = 0.0001; r = 1.0  # base 0.74 / 0.8 / +-0.03 even better than best
    # q = 0.00005; r = 2.0  # base 0.74 / 0.8 / +-0.03 even better than best
    # q = 0.00001; r = 1.0  # base 0.74 / 0.8 / +-0.03 even better than best
    # q = 0.00005; r = 1.0  # base 0.74 / 0.8 / +-0.03 even better than best
    # q = 0.00005; r = 1.0; sres = 2.  # base 0.74 / 0.8 / +-0.03 even better than best
    # q = 0.00005; r = 1.0; sres = 1.; tau_dif = 83./2.  # base 0.74 / 0.8 / +-0.03 even better than best
    # q = 0.00005; r = 1.0; sres = 2.; tau_dif = 83./2.  # base 0.74 / 0.8 / +-0.03 even better than best
    # q = 0.00005; r = 1.0; sres = 1.; tau_dif = 83.*4.  # base 0.74 / 0.8 / +-0.03 even better than best
    # q = 0.00005; r = 1.0; sres = 1.; tau_dif = 83.; tau_sd = 1.8e7*1.4   # base 0.74 / 0.8 / +-0.03 even better than best
    # q = 0.00005; r = 1.0; tau_sd = 2.53e7   # base 0.74 / 0.8 / +-0.03 even better than best
    q = 0.00005; r = 1.0; tau_sd = 2.5e7;   # base 0.74 / 0.8 / +-0.03 even better than best
    sim = BatteryModel(temp_c=temp_c, tau_ct=tau_ct, scale=scale, hys_scale=hys_scale, tweak_test=tweak_test,
                       dv_hys=dv_hys, sres=sres)
    mon = BatteryMonitor(r_sd=rsd, tau_sd=tau_sd, r0=r0, tau_ct=tau_ct, r_ct=rct, tau_dif=tau_dif, r_dif=r_dif,
                         temp_c=temp_c, hys_scale=hys_scale_monitor, tweak_test=tweak_test, dv_hys=dv_hys, sres=sres)
    # need Tb input.   perhaps need higher order to enforce basic type 1 response
    mon.Q = q * q
    mon.R = r * r
    Is_sat_delay = TFDelay(in_=saved_old.soc[0] > 0.97, t_true=T_SAT, t_false=T_DESAT, dt=0.1)  # later, dt is changed

    # time loop
    T = t[1] - t[0]
    for i in range(t_len):
        saved_old.i = i
        if i > 0:
            T = t[i] - t[i - 1]

        # dc_dc_on = bool(lut_dc.interp(t[i]))
        dc_dc_on = False
        reset = (t[i] <= init_time)

        if reset:
            sim.apply_soc(soc_m_init, Tb[i])
            rp.delta_q_model = sim.delta_q
            rp.t_last_model = Tb[i] + dt_model
            sim.load(rp.delta_q_model, rp.t_last_model)
            sim.init_battery()
            sim.apply_delta_q_t(rp.delta_q_model, rp.t_last_model)

        # Models
        if t_Ib_fail and t[i] > t_Ib_fail:
            ib_in_m = Ib_fail
            current_in = Ib_fail
        else:
            if saved_sim_old:
                ib_in_m = saved_sim_old.ib_in_m[i]
            else:
                ib_in_m = Ib_past[i]
            current_in = saved_old.Ib[i]
        sim.calculate(temp_c=Tb[i], soc=sim.soc, curr_in=ib_in_m, dt=T, q_capacity=sim.q_capacity,
                      dc_dc_on=dc_dc_on, reset=reset, rp=rp)
        if t_Ib_fail and t[i] > t_Ib_fail:
            charge_curr = Ib_fail
        else:
            charge_curr = sim.ib
        sim.count_coulombs(dt=T, reset=reset, temp_c=Tb[i], charge_curr=charge_curr, sat=False, soc_m_init=soc_m_init,
                           mon_sat=mon.sat, mon_delta_q=mon.delta_q)

        # EKF
        if reset:
            mon.apply_soc(soc_init, Tb[i])
            rp.delta_q = mon.delta_q
            rp.t_last = Tb[i]
            mon.load(rp.delta_q, rp.t_last)
            mon.assign_temp_c(Tb[i])
            mon.init_battery()
            mon.init_soc_ekf(soc_init)  # when modeling (assumed in python) ekf wants to equal model

        # Monitor calculations including ekf
        if t_Vb_fail and t[i] >= t_Vb_fail:
            Vb_ = Vb_fail
            vb = Vb_fail
        else:
            Vb_ = Vb[i]
            vb = sim.vb
        if rp.modeling == 0:
            mon.calculate(Tb[i], Vb_, current_in, T, rp=rp, reset=reset)
        else:
            mon.calculate(Tb[i], vb + randn() * v_std + dv_sense, charge_curr + randn() * i_std + di_sense, T, rp=rp,
                          reset=reset, d_voc=None)
            # mon.calculate(Tb[i], vb + randn() * v_std + dv_sense, charge_curr + randn() * i_std + di_sense, T, rp=rp,
            #           reset=reset, d_voc=Voc[i])
        # mon.calculate(Tb[i], Vb[i]+randn()*v_std+dv_sense, sim.ib+randn()*i_std+di_sense, T)
        sat = is_sat(Tb[i], mon.voc, mon.soc)
        saturated = Is_sat_delay.calculate(sat, T_SAT, T_DESAT, min(T, T_SAT / 2.), reset)
        if rp.modeling == 0:
            mon.count_coulombs(dt=T, reset=reset, temp_c=Tb[i], charge_curr=current_in, sat=saturated)
        else:
            mon.count_coulombs(dt=T, reset=reset, temp_c=temp_c, charge_curr=charge_curr, sat=saturated)
        mon.calc_charge_time(mon.q, mon.q_capacity, charge_curr, mon.soc)
        mon.select()
        # mon.regauge(Tb[i])
        mon.assign_soc_m(sim.soc)

        # Plot stuff
        mon.save(t[i], T, mon.soc, sim.voc)
        sim.save(t[i], T)
        sim.save_m(t[i], T)

        # Print initial
        if i == 0:
            print('time=', t[i])
            print('mon:  ', str(mon))
            print('time=', t[i])
            print('sim:  ', str(sim))

        # print("Tb[i], t_last, t_last_model", Tb[i], mon.t_last, sim.t_last)

    # Data
    print('time=', t[i])
    print('mon:  ', str(mon))
    print('sim:  ', str(sim))

    # print("Showing data from file then BatteryMonitor calculated from data")
    # print(compare_print(saved_old, mon.saved))

    return mon.saved, sim.saved, mon.Randles.saved, sim.saved_m

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
        time_end = 5.
        # time_end = 2000.

        # Setup and user inputs (data_file_old_txt must end in .txt)
        # data_file_old_txt = '../dataReduction/tryXp20_20220626.txt';unit_key = 'pro_2022';
        # data_file_old_txt = '../dataReduction/real world Xp20 20220626.txt';unit_key = 'soc0_2022';
        # data_file_old_txt = '../dataReduction/real world Xp21 20220626.txt';unit_key = 'soc0_2022';
        # data_file_old_txt = '../dataReduction/rapidTweakRegressionTest20220710.txt';unit_key = 'pro_2022' TODO: delete
        data_file_old_txt = '../dataReduction/rapidTweakRegressionTest20220711.txt';unit_key = 'pro_2022'
        title_key = "unit,"  # Find one instance of title
        title_key_sim = "unit_m,"  # Find one instance of title
        unit_key_sim = "unit_sim"

        # Load mon v4 (old)
        data_file_clean = write_clean_file(data_file_old_txt, type='_mon', title_key=title_key, unit_key=unit_key)
        cols = ('unit', 'hm', 'cTime', 'dt', 'sat', 'sel', 'mod', 'Tb', 'Vb', 'Ib', 'Vsat', 'dV_dyn', 'Voc_stat',
                'Voc_ekf', 'y_ekf', 'soc_m', 'soc_ekf', 'soc', 'soc_wt')
        data_old = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols, dtype=None,
                                 encoding=None).view(np.recarray)
        saved_old = SavedData(data=data_old, time_end=time_end)

        # Load _m v24 portion of real-time run (old)
        data_file_sim_clean = write_clean_file(data_file_old_txt, type='_sim', title_key=title_key_sim, unit_key=unit_key_sim)
        cols_sim = ('unit_m', 'c_time', 'Tb_m', 'Tbl_m', 'vsat_m', 'voc_stat_m', 'dv_dyn_m', 'vb_m', 'ib_m', 'ib_in_m',
                    'sat_m', 'ddq_m', 'dq_m', 'q_m', 'qcap_m', 'soc_m', 'reset_m')
        if data_file_sim_clean:
            data_sim_old = np.genfromtxt(data_file_sim_clean, delimiter=',', names=True, usecols=cols_sim, dtype=None,
                                 encoding=None).view(np.recarray)
            saved_sim_old = SavedDataSim(time_ref=saved_old.time_ref, data=data_sim_old, time_end=time_end)
        else:
            saved_sim_old = None

        # How to initialize
        if saved_old.time[0] == 0.: # no initialization flat detected at beginning of recording
            init_time = 1.
        else:
            init_time = -4.
        # Get dv_hys from data
        dv_hys = saved_old.dV_hys[0]

        # New run
        mon_file_save = data_file_clean.replace(".csv", "_rep.csv")
        mons, sims, monrs, sims_m = replicate(saved_old, saved_sim_old=saved_sim_old, init_time=init_time, dv_hys=dv_hys, sres=1.0)
        save_clean_file(mons, mon_file_save, 'mon_rep' + date_)

        # Plots
        n_fig = 0
        fig_files = []
        data_root = data_file_clean.split('/')[-1].replace('.csv', '-')
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        n_fig, fig_files = overalls(mons, sims, monrs, filename, fig_files, plot_title=plot_title, n_fig=n_fig)  # sim over mon
        n_fig, fig_files = overall(saved_old, mons, saved_sim_old, sims, sims_m, filename, fig_files, plot_title=plot_title, n_fig=n_fig,
                                   new_s_s=sims)  # mon over data
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='../dataReduction/figures')
        cleanup_fig_files(fig_files)

        plt.show()

    main()
