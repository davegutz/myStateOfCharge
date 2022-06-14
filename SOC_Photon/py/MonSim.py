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
    cols = ('unit', 'hm', 'cTime', 'dt', 'sat', 'sel', 'mod', 'Tb', 'Vb', 'Ib', 'Vsat', 'Vdyn', 'Voc', 'Voc_ekf',
            'y_ekf', 'soc_m', 'soc_ekf', 'soc', 'soc_wt')
    default_header_str = "unit,               hm,                  cTime,        dt,       sat,sel,mod,  Tb,  Vb,  Ib,        Vsat,Vdyn,Voc,Voc_ekf,     y_ekf,    soc_m,soc_ekf,soc,soc_wt,"
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
            s += "{:7.3f},".format(mons.Vdyn[i])
            s += "{:7.3f},".format(mons.Voc[i])
            s += "{:7.3f},".format(mons.Voc_ekf[i])
            s += "{:7.3f},".format(mons.y_ekf[i])
            s += "{:7.3f},".format(mons.soc_m[i])
            s += "{:7.3f},".format(mons.soc_ekf[i])
            s += "{:7.3f},".format(mons.soc[i])
            s += "{:7.3f},".format(mons.soc_wt[i])
            s += "\n"
            output.write(s)
        print("Wrote ", csv_file)

def replicate(saved_old):
    t = saved_old.time
    dt = saved_old.dt
    Vb = saved_old.Vb
    Ib = saved_old.Ib
    Tb = saved_old.Tb
    t_len = len(t)
    rp = Retained()
    rp.modeling = saved_old.mod()
    print("rp.modeling is ", rp.modeling)
    tweak_test = rp.tweak_test()
    temp_c = saved_old.Tb[0]

    # Setup
    scale = model_bat_cap / Battery.RATED_BATT_CAP
    sim = BatteryModel(temp_c=temp_c, tau_ct=tau_ct, scale=scale, hys_scale=hys_scale, tweak_test=tweak_test)
    mon = BatteryMonitor(r_sd=rsd, tau_sd=tau_sd, r0=r0, tau_ct=tau_ct, r_ct=rct, tau_dif=tau_dif,
                         r_dif=r_dif, temp_c=temp_c, hys_scale=hys_scale_monitor, tweak_test=tweak_test)
    Is_sat_delay = TFDelay(in_=saved_old.soc[0] > 0.97, t_true=T_SAT, t_false=T_DESAT, dt=0.1)  # later, dt is changed

    # time loop
    T = t[1] - t[0]
    for i in range(t_len):
        saved_old.i = i
        current_in = saved_old.Ib[i]
        if i > 0:
            T = t[i] - t[i - 1]

        # dc_dc_on = bool(lut_dc.interp(t[i]))
        dc_dc_on = False
        init = (t[i] <= 1)

        if init:
            sim.apply_soc(soc_init, temp_c)
            rp.delta_q_model = sim.delta_q
            rp.t_last_model = temp_c + dt_model
            sim.load(rp.delta_q_model, rp.t_last_model)
            sim.init_battery()
            sim.apply_delta_q_t(rp.delta_q_model, rp.t_last_model)

        # Models
        sim.calculate(temp_c=temp_c, soc=sim.soc, curr_in=current_in, dt=T, q_capacity=sim.q_capacity,
                      dc_dc_on=dc_dc_on, rp=rp)
        sim.count_coulombs(dt=T, reset=init, temp_c=temp_c, charge_curr=sim.ib, t_last=rp.t_last_model,
                           sat=False)
        rp.delta_q_model, rp.t_last_model = sim.update()

        # EKF
        if init:
            mon.apply_soc(soc_init, temp_c)
            rp.delta_q = mon.delta_q
            rp.t_last = temp_c
            mon.load(rp.delta_q, rp.t_last)
            mon.assign_temp_c(temp_c)
            mon.init_battery()
            mon.init_soc_ekf(soc_init)  # when modeling (assumed in python) ekf wants to equal model

        # Monitor calculations including ekf
        if rp.modeling == 0:
            mon.calculate(Tb[i], Vb[i], Ib[i], T, rp=rp)
        else:
            mon.calculate(temp_c, sim.vb + randn() * v_std + dv_sense, sim.ib + randn() * i_std + di_sense, T, rp=rp)
        # mon.calculate(temp_c, Vb[i]+randn()*v_std+dv_sense, sim.ib+randn()*i_std+di_sense, T)
        sat = is_sat(temp_c, mon.voc, mon.soc)
        saturated = Is_sat_delay.calculate(sat, T_SAT, T_DESAT, min(T, T_SAT / 2.), init)
        if rp.modeling == 0:
            mon.count_coulombs(dt=T, reset=init, temp_c=Tb[i], charge_curr=Ib[i], sat=saturated,
                               t_last=mon.t_last)
        else:
            mon.count_coulombs(dt=T, reset=init, temp_c=temp_c, charge_curr=sim.ib, sat=saturated,
                               t_last=mon.t_last)
        mon.calc_charge_time(mon.q, mon.q_capacity, mon.ib, mon.soc)
        mon.select()
        # mon.regauge(temp_c)
        mon.assign_soc_m(sim.soc)
        rp.delta_q, rp.t_last = mon.update()

        # Plot stuff
        mon.save(t[i], T, mon.soc, sim.voc)
        sim.save(t[i], T, sim.soc, sim.voc)

        # Print init
        if i == 0:
            print('time=', t[i])
            print('mon:  ', str(mon))
            print('time=', t[i])
            print('sim:  ', str(sim))

    # Data
    print('time=', t[i])
    print('mon:  ', str(mon))
    print('sim:  ', str(sim))
    # print("Showing data from file then BatteryMonitor calculated from data")
    # print(compare_print(saved_old, mon.saved))

    return mon.saved, sim.saved, mon.Randles.saved

if __name__ == '__main__':
    import sys
    from DataOverModel import SavedData, write_clean_file, overall
    from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
    import matplotlib.pyplot as plt

    def main():
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        date_ = datetime.now().strftime("%y%m%d")

        # Transient  inputs
        time_end = None
        # time_end = 2500.

        # Load data (must end in .txt)
        data_file_old_txt = '../dataReduction/rapidTweakRegressionTest20220613_new.txt'; unit_key = 'pro_2022'
        title_key = "unit,"  # Find one instance of title
        data_file_clean = write_clean_file(data_file_old_txt, title_key, unit_key)

        # Load
        cols = ('unit', 'hm', 'cTime', 'dt', 'sat', 'sel', 'mod', 'Tb', 'Vb', 'Ib', 'Vsat', 'Vdyn', 'Voc', 'Voc_ekf',
                'y_ekf', 'soc_m', 'soc_ekf', 'soc', 'soc_wt')
        data_old = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols, dtype=None,
                                 encoding=None).view(np.recarray)
        saved_old = SavedData(data_old, time_end)
        mon_file_save = data_file_clean.replace(".csv", "_rep.csv")
        mons, sims, monrs = replicate(saved_old)
        save_clean_file(mons, mon_file_save, 'rep' + date_)

        # Plots
        n_fig = 0
        fig_files = []
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        n_fig, fig_files = overalls(mons, sims, monrs, filename, fig_files,plot_title=plot_title, n_fig=n_fig)
        n_fig, fig_files = overall(saved_old, mons, filename, fig_files, plot_title=plot_title, n_fig=n_fig,
                                   new_s_s=sims)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='../dataReduction/figures')
        cleanup_fig_files(fig_files)

        plt.show()

    main()
