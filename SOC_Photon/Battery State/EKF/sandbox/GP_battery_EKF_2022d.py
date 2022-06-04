# GP_batteryEKF - general purpose battery class for EKF use
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

"""Define a general purpose battery model including Randle's model and SoC-VOV model as well as Kalman filtering
support for simplified Mathworks' tracker. See Huria, Ceraolo, Gazzarri, & Jackey, 2013 Simplified Extended Kalman
Filter Observer for SOC Estimation of Commercial Power-Oriented LFP Lithium Battery Cells """

import numpy as np
from numpy.random import randn
import Battery
from Battery import Battery, BatteryMonitor, BatteryModel, is_sat, rp, overall
from unite_pictures import unite_pictures_into_pdf
import os
from datetime import datetime
from TFDelay import TFDelay

if __name__ == '__main__':
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt
    import book_format
    from pyDAGx import myTables
    book_format.set_style()


    class SavedData:
        def __init__(self, data=None, time_end=None):
            if data is None:
                self.time = []
                self.Ib = []  # Bank current, A
                self.Vb = []  # Bank voltage, V
                self.sat = []  # Indication that battery is saturated, T=saturated
                self.sel = []  # Current source selection, 0=amp, 1=no amp
                self.mod = []  # Configuration control code, 0=all hardware, 7=all simulated, +8 tweak test
                self.Tb = []  # Battery bank temperature, deg C
                self.Vsat = []  # Monitor Bank saturation threshold at temperature, deg C
                self.Vdyn = []  # Monitor Bank current induced back emf, V
                self.Voc = []  # Monitor Static bank open circuit voltage, V
                self.Voc_ekf = []  # Monitor bank solved static open circuit voltage, V
                self.y_ekf = []  # Monitor single battery solver error, V
                self.soc_m = []  # Simulated state of charge, fraction
                self.soc_ekf = []  # Solved state of charge, fraction
                self.soc = []  # Coulomb Counter fraction of saturation charge (q_capacity_) availabel (0-1)
                self.soc_wt = []  # Weighted selection of ekf state of charge and Coulomb Counter (0-1)
            else:
                self.time = data.cTime
                self.Ib = data.Ib
                # manage data shape
                # Find first non-zero Ib and use to adjust time
                # Ignore initial run of non-zero Ib because resetting from previous run
                zero_start = np.where(data.Ib == 0.0)[0][0]
                self.zero_end = zero_start
                while data.Ib[self.zero_end] == 0.0:  # stop at first non-zero
                    self.zero_end += 1
                time_ref = self.time[self.zero_end]
                print("time_ref=", time_ref)
                self.time -= time_ref
                # Truncate
                if time_end is None:
                    i_end = len(self.time)
                else:
                    i_end = np.where(self.time <= time_end)[0][-1]+1
                self.time = data.cTime[:i_end]
                self.Ib = data.Ib[:i_end]
                self.Vb = data.Vb[:i_end]
                self.sat = data.sat[:i_end]
                self.sel = data.sel[:i_end]
                self.mod_data = data.mod[:i_end]
                self.Tb = data.Tb[:i_end]
                self.Vsat = data.Vsat[:i_end]
                self.Vdyn = data.Vdyn[:i_end]
                self.Voc = data.Voc[:i_end]
                self.Voc_ekf = data.Voc_ekf[:i_end]
                self.y_ekf = data.y_ekf[:i_end]
                self.soc_m = data.soc_m[:i_end]
                self.soc_ekf = data.soc_ekf[:i_end]
                self.soc = data.soc[:i_end]
                self.soc_wt = data.soc_wt[:i_end]

        def mod(self):
             return self.mod_data[self.zero_end]

        def overall(old_s, new_s, filename, fig_files=None, plot_title=None, n_fig=None):
            if fig_files is None:
                fig_files = []
            plt.figure()
            n_fig += 1
            plt.subplot(221)
            plt.title(plot_title)
            plt.plot(old_s.time, old_s.Ib, color='green', label='Ib')
            plt.plot(new_s.time, new_s.Ib, color='orange', linestyle='--', label='Ib_new')
            plt.legend(loc=1)
            plt.subplot(222)
            plt.plot(old_s.time, old_s.sat, color='black', label='sat')
            plt.plot(new_s.time, new_s.sat, color='orange', linestyle='--', label='sat_new')
            plt.plot(old_s.time, old_s.sel, color='red', label='sel')
            plt.plot(new_s.time, new_s.sel, color='blue', linestyle='--', label='sel_new')
            plt.plot(old_s.time, old_s.mod_data, color='blue', label='mod')
            plt.plot(new_s.time, new_s.mod_data, color='red', linestyle='--', label='mod_new')
            plt.legend(loc=1)
            plt.subplot(223)
            plt.plot(old_s.time, old_s.Vb, color='green', label='Vb')
            plt.plot(new_s.time, new_s.Vb, color='orange', linestyle='--', label='Vb_new')
            plt.legend(loc=1)
            plt.subplot(224)
            plt.plot(old_s.time, old_s.Voc, color='green', label='Voc')
            plt.plot(new_s.time, new_s.Voc, color='orange', linestyle='--', label='Voc_new')
            plt.plot(old_s.time, old_s.Vsat, color='blue', label='Vsat')
            plt.plot(new_s.time, new_s.Vsat, color='red', linestyle='--', label='Vsat_new')
            plt.legend(loc=1)
            fig_file_name = filename + '_' + str(n_fig) + ".png"
            fig_files.append(fig_file_name)
            plt.savefig(fig_file_name, format="png")

            plt.figure()
            n_fig += 1
            plt.subplot(221)
            plt.title(plot_title)
            plt.plot(old_s.time, old_s.Vdyn, color='green', label='Vdyn')
            plt.plot(new_s.time, new_s.Vdyn, color='orange', linestyle='--', label='Vdyn_new')
            plt.legend(loc=1)
            plt.subplot(222)
            plt.plot(old_s.time, old_s.Voc, color='green', label='Voc')
            plt.plot(new_s.time, new_s.Voc, color='orange', linestyle='--', label='Voc_new')
            plt.legend(loc=1)
            plt.subplot(223)
            plt.plot(old_s.time, old_s.Voc, color='green', label='Voc')
            plt.plot(new_s.time, new_s.Voc, color='orange', linestyle='--', label='Voc_new')
            plt.plot(old_s.time, old_s.Voc_ekf, color='blue', label='Voc_ekf')
            plt.plot(new_s.time, new_s.Voc_ekf, color='red', linestyle='--', label='Voc_ekf_new')
            plt.legend(loc=1)
            plt.subplot(224)
            plt.plot(old_s.time, old_s.y_ekf, color='green', label='y_ekf')
            plt.plot(new_s.time, new_s.y_ekf, color='orange', linestyle='--', label='y_ekf_new')
            plt.legend(loc=1)
            fig_file_name = filename + '_' + str(n_fig) + ".png"
            fig_files.append(fig_file_name)
            plt.savefig(fig_file_name, format="png")

            plt.figure()
            n_fig += 1
            plt.subplot(221)
            plt.title(plot_title)
            plt.plot(old_s.time, old_s.soc, color='blue', label='soc')
            plt.plot(new_s.time, new_s.soc, color='red', linestyle='--', label='soc_new')
            plt.legend(loc=1)
            plt.subplot(222)
            plt.plot(old_s.time, old_s.soc_ekf, color='green', label='soc_ekf')
            plt.plot(new_s.time, new_s.soc_ekf, color='orange', linestyle='--', label='soc_ekf_new')
            plt.legend(loc=1)
            plt.subplot(223)
            plt.plot(old_s.time, old_s.soc_wt, color='green', label='soc_wt')
            plt.plot(new_s.time, new_s.soc_wt, color='orange', linestyle='--', label='soc_wt_new')
            plt.plot(old_s.time, old_s.soc, color='blue', label='soc')
            plt.plot(new_s.time, new_s.soc, color='red', linestyle='--', label='soc_new')
            plt.legend(loc=1)
            plt.subplot(224)
            plt.plot(old_s.time, old_s.soc_m, color='green', label='soc_m')
            plt.plot(new_s.time, new_s.soc_m, color='orange', linestyle='--', label='soc_m_new')
            plt.legend(loc=1)
            fig_file_name = filename + '_' + str(n_fig) + ".png"
            fig_files.append(fig_file_name)
            plt.savefig(fig_file_name, format="png")

            return n_fig, fig_files


    def main():
        # Trade study inputs
        # i-->0 provides continuous anchor to reset filter (why?)  i shifts important --> 2 current sensors, hyst in ekf
        # saturation provides periodic anchor to reset filter
        # reset soc periodically anchor user display
        # tau_sd creating an anchor.   So large it's just a pass through.  TODO:  Why x correct??
        # TODO:  filter soc for saturation calculation in model
        # TODO:  temp sensitivities and mitigation
        dv_sense = 0.  # (0.-->0.1) ++++++++ flat curve
        di_sense = 0.  # (0.-->0.5) ++++++++  i does not go to zero steady state
        dt_model = 0.  # (0.-->25.) +++++++  TODO:   ?????????
        model_bat_cap = 100.  # (100.-->80) ++++++++++ dyn only provided reset soc periodically  TODO:  ???????
        r0 = 0.003  # (0.003-->0.006)  +++ dyn only provided reset soc periodically
        rct = 0.0016  # (0.0016-->0.0032) ++++++++++ dyn only provided reset soc periodically
        tau_dif = 83  # (83-->100)  +++++++++++ dyn only provided reset soc periodically
        r_dif = 0.0077  # (0.0077-->0.015)   ++++++++++  dyn only provided reset soc periodically
        rsd = 70.  # (70.-->700)  ------- dyn only
        tau_ct = 0.2  # (0.2-->5.)  -------
        tau_sd = 1.8e7  # (1.8e7-->1.8e6) ++++++ dyn only
        v_std = 0.  # (0.01-->0) ------ noise
        i_std = 0.  # (0.1-->0) ------ noise
        soc_init = 1.0  # (1.0-->0.8)  ------  initialization artifacts only
        hys_scale = 1.  # (1e-6<--1.-->10.) 1e-6 disables hysteresis
        hys_scale_monitor = -1.  # (-1e-6<-- -1.-->-10.) -1e-6 disables hysteresis.   Negative reverses hys
        T_SAT = 5.  # Saturation time, sec
        T_DESAT = T_SAT * 2.  # De-saturation time, sec

        # Transient  inputs
        # Current time inputs representing the load.
        t_x_i = [0.0, 49.9, 50.0,  449.9, 450., 999.9, 1000., 2999.9, 3000.]  # seconds
        t_i = [0.0,   0.0,  -40.0, -40.0, 0.0,  0.0,   40.0,  40.0,   0.0]  # Amperes
        # DC-DC charger status.   0=off, 1=on
        t_x_d = [0.0, 199., 200.0,  299.9, 300.0]
        t_d = [0,     0,    1,      1,     0]
        time_end = None
        # time_end = 1.

        # Load data
        data_file_old = '../../../dataReduction/rapidTweakRegressionTest20220529_old.csv'
        cols = ('unit', 'cTime', 'T', 'sat', 'sel', 'mod', 'Tb', 'Vb', 'Ib', 'Vsat', 'Vdyn', 'Voc', 'Voc_ekf',
                'y_ekf', 'soc_m', 'soc_ekf', 'soc', 'soc_wt')
        # noinspection PyTypeChecker
        data_old = np.genfromtxt(data_file_old, delimiter=',', names=True, usecols=cols, dtype=None,
                                 encoding=None).view(np.recarray)
        saved_old = SavedData(data_old, time_end)
        t = saved_old.time
        t_len = len(t)
        rp.modeling = saved_old.mod()
        temp_c = data_old.Tb[0]

        # Setup
        lut_i = myTables.TableInterp1D(np.array(t_x_i), np.array(t_i))
        lut_dc = myTables.TableInterp1D(np.array(t_x_d), np.array(t_d))
        scale = model_bat_cap / Battery.RATED_BATT_CAP
        sim = BatteryModel(temp_c=temp_c, tau_ct=tau_ct, scale=scale, hys_scale=hys_scale)
        mon = BatteryMonitor(r_sd=rsd, tau_sd=tau_sd, r0=r0, tau_ct=tau_ct, r_ct=rct, tau_dif=tau_dif,
                      r_dif=r_dif, temp_c=temp_c, hys_scale=hys_scale_monitor)
        Is_sat_delay = TFDelay(in_=data_old.soc[0]>0.97, t_true=T_SAT, t_false=T_DESAT, dt=0.1)  # later, dt is changed

        # time loop
        dt = t[1] - t[0]
        dt_ekf = t[1] - t[0]
        for i in range(t_len):
            current_in = saved_old.Ib[i]
            if i>0:
                dt = t[i] - t[i-1]
                dt_ekf = dt

            # dc_dc_on = bool(lut_dc.interp(t[i]))
            dc_dc_on = False
            init = (t[i] <= 1)

            if init:
                sim.apply_soc(soc_init, temp_c)
                rp.delta_q_model = sim.delta_q
                rp.t_last_model = temp_c+dt_model
                sim.load(rp.delta_q_model, rp.t_last_model)
                sim.init_battery()
                sim.apply_delta_q_t(rp.delta_q_model, rp.t_last_model)

            # Models
            sim.calculate(temp_c=temp_c, soc=sim.soc, curr_in=current_in, dt=dt, q_capacity=sim.q_capacity,
                          dc_dc_on=dc_dc_on)
            sim.count_coulombs(dt=dt, reset=init, temp_c=temp_c, charge_curr=sim.ib, t_last=rp.t_last_model,
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
            mon.calculate(temp_c, sim.vb+randn()*v_std+dv_sense, sim.ib+randn()*i_std+di_sense, dt_ekf)
            sat = is_sat(temp_c, mon.voc, mon.soc)
            saturated = Is_sat_delay.calculate(sat, T_SAT, T_DESAT, min(dt, T_SAT/2.), init)
            mon.count_coulombs(dt=dt_ekf, reset=init, temp_c=temp_c, charge_curr=sim.ib,
                               sat=saturated, t_last=mon.t_last)
            mon.calc_charge_time(mon.q, mon.q_capacity, mon.ib, mon.soc)
            mon.select()
            mon.assign_soc_m(sim.soc)
            rp.delta_q, rp.t_last = mon.update()

            # Plot stuff
            mon.save(t[i], mon.soc, sim.voc)
            sim.save(t[i], sim.soc, sim.voc)

            # Print end of init
            # if i<300 and t[i+1]==0. and t[i]<0.:
            if i==0:
                print('time=', t[i])
                print('mon:  ', str(mon))
                print('time=', t[i])
                print('sim:  ', str(sim))

        # Data
        print('time=', t[i])
        print('mon:  ', str(mon))
        print('sim:  ', str(sim))


        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time

        n_fig, fig_files = overall(mon.saved, sim.saved, filename, fig_files,plot_title=plot_title, n_fig=n_fig)
        n_fig, fig_files = SavedData.overall(saved_old, mon.saved, filename, fig_files, plot_title=plot_title, n_fig=n_fig)

        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='figures')

        # Clean up after itself.   Other fig files already in root will get plotted by unite_pictures_into_pdf
        # Cleanup other figures in root folder by hand
        for fig_file in fig_files:
            try:
                os.remove(fig_file)
            except OSError:
                pass
        plt.show()

    main()
