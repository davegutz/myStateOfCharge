# GP_batteryEKF - general purpose battery class for EKF use
# Copyright (C) 2021 Dave Gutz
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
from Battery import Battery, BatteryModel, is_sat, rp, overall
from unite_pictures import unite_pictures_into_pdf
import os
from datetime import datetime

if __name__ == '__main__':
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt
    import book_format
    from pyDAGx import myTables

    book_format.set_style()


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
        tau_sd = 1.87e7  # (1.87e7-->1.87e6) ++++++ dyn only
        v_std = 0.01  # (0.01-->0) ------ noise
        i_std = 0.1  # (0.1-->0) ------ noise
        soc_init = 1.0  # (1.0-->0.8)  ------  initialization artifacts only
        tau_ct = 0.2  # (0.2-->5.)  -------
        hys_scale = 1.  # (1.-->10.)

        # Transient  inputs
        # Current time inputs representing the load.
        t_x_i = [0.0, 49.9, 50.0,  449.9, 450., 999.9, 1000., 2999.9, 3000.]  # seconds
        t_i = [0.0,   0.0,  -40.0, -40.0, 0.0,  0.0,   40.0,  40.0,   0.0]  # Amperes
        # DC-DC charger status.   0=off, 1=on
        t_x_d = [0.0, 199., 200.0,  299.9, 300.0]
        t_d = [0,     0,    1,      1,     0]
        # time_end = 2
        # time_end = 13.3
        # time_end = 700
        time_end = 3500
        # time_end = 800
        temp_c = 25.
        # temp_c = 0.

        # Setup
        dt = 0.1
        dt_ekf = 0.1
        lut_i = myTables.TableInterp1D(np.array(t_x_i), np.array(t_i))
        lut_dc = myTables.TableInterp1D(np.array(t_x_d), np.array(t_d))
        scale = model_bat_cap / Battery.RATED_BATT_CAP
        sim = BatteryModel(temp_c=temp_c, tau_ct=tau_ct, scale=scale, hys_scale=hys_scale)
        mon = Battery(r_sd=rsd, tau_sd=tau_sd, r0=r0, tau_ct=tau_ct, r_ct=rct, tau_dif=tau_dif,
                      r_dif=r_dif, temp_c=temp_c)

        # time loop
        t = np.arange(0, time_end + dt, dt)
        for i in range(len(t)):
            current_in = lut_i.interp(t[i])
            dc_dc_on = bool(lut_dc.interp(t[i]))
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
            mon.calculate_ekf(temp_c, sim.vb+randn()*v_std+dv_sense, sim.ib+randn()*i_std+di_sense, dt_ekf)
            mon.count_coulombs(dt=dt_ekf, reset=init, temp_c=temp_c, charge_curr=sim.ib,
                               sat=is_sat(temp_c, mon.voc, mon.soc), t_last=mon.t_last)
            mon.calc_charge_time(mon.q, mon.q_capacity, mon.ib, mon.soc)
            rp.delta_q, rp.t_last = mon.update()

            # Plot stuff
            mon.save(t[i], sim.soc, sim.voc)
            sim.save(t[i], sim.soc, sim.voc)

        # Data
        print('mon:  ', str(mon))
        print('sim:  ', str(sim))

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time

        n_fig, fig_files = overall(mon.saved, sim.saved, filename, fig_files,
                                   plot_title=plot_title, n_fig=n_fig)

        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='figures')
        for fig_file in fig_files:
            try:
                os.remove(fig_file)
            except OSError:
                pass
        plt.show()

    main()
