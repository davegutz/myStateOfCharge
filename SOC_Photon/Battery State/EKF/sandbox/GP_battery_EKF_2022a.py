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

    book_format.set_style()


    def main():
        # Setup to run the transients
        dt = 0.1
        dt_ekf = 0.1
        # time_end = 2
        # time_end = 13.3
        # time_end = 700
        time_end = 3500
        # time_end = 800
        temp_c = 25.
        # temp_c = 0.

        # Trade study inputs
        # i-->0 provides continuous anchor to reset filter (why?)  i shifts important --> 2 current sensors, hyst in ekf
        # saturation provides periodic anchor to reset filter
        # reset soc periodically anchor user display
        # tau_sd creating an anchor.   So large it's just a pass through.  TODO:  Why x correct??
        # TODO:  filter soc for saturation calculation in model
        # TODO:  temp sensitivities and mitigation
        dv_sense = 0.  # (0.-->0.1) ++++++++ flat curve
        di_sense = 0.  # (0.-->0.5) ++++++++  i does not go to zero steady state
        i_hyst = 0.  # (0.-->5.) ++++++++  dyn only since i->0 steady state.  But very large transiently
        dt_model = 0.  # (0.-->25.) +++++++  TODO:   ?????????
        model_bat_cap = 100.  # (100.-->80) ++++++++++ dyn only provided reset soc periodically  TODO:  ???????
        soc_init_err = 0.0  # (0-->-0.2)  ------ long simulation init time corrects this --> reset soc periodically
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
        t_dc_dc_on = 0  # (1e6-->0)  1e6 for never on, 0 for always on

        # Setup
        r_std = 0.1  # Kalman sensor uncertainty (0.1) belief in meas
        q_std = 0.001  # Process uncertainty (0.001) belief in state
        scale = model_bat_cap / Battery.RATED_BATT_CAP
        sim = BatteryModel(temp_c=temp_c, tau_ct=tau_ct, scale=scale, hys_scale=hys_scale)
        mon = Battery(r_sd=rsd, tau_sd=tau_sd, r0=r0, tau_ct=tau_ct, r_ct=rct, tau_dif=tau_dif,
                      r_dif=r_dif, temp_c=temp_c)

        # Executive tasks
        t = np.arange(0, time_end + dt, dt)
        current_in_s = []

        # time loop
        for i in range(len(t)):
            if t[i] < 50:
                current_in = 0.
            elif t[i] < 450:
                current_in = -40.
            elif t[i] < 1000:
                current_in = 0.
            elif t[i] < 3000:
                current_in = 40.
            else:
                current_in = 0.
            if t[i] >= t_dc_dc_on:
                dc_dc_on = True
            else:
                dc_dc_on = False
            init_ekf = (t[i] <= 1)

            if init_ekf:
                sim.apply_soc(soc_init, temp_c)
                rp.delta_q_model = sim.delta_q
                rp.t_last_model = temp_c+dt_model
                sim.load(rp.delta_q_model, rp.t_last_model)
                sim.init_battery()
                sim.apply_delta_q_t(rp.delta_q_model, rp.t_last_model)

            # Models
            sim.calculate(temp_c=temp_c, soc=sim.soc, curr_in=current_in, dt=dt, q_capacity=sim.q_capacity,
                          dc_dc_on=dc_dc_on)
            sim.count_coulombs(dt=dt, reset=init_ekf, temp_c=temp_c, charge_curr=sim.ib, t_last=rp.t_last_model,
                               sat=False)
            rp.delta_q_model, rp.t_last_model = sim.update()

            # EKF
            if init_ekf:
                mon.apply_soc(soc_init, temp_c)
                rp.delta_q = mon.delta_q
                rp.t_last = temp_c
                mon.load(rp.delta_q, rp.t_last)
                mon.assign_temp_c(temp_c)
                mon.init_battery()
                mon.init_soc_ekf(soc_init)  # when modeling (assumed in python) ekf wants to equal model

            # Monitor calculations including ekf
            mon.calculate_ekf(temp_c, sim.vb+randn()*v_std+dv_sense, sim.ib+randn()*i_std+di_sense, dt_ekf)
            mon.count_coulombs(dt=dt_ekf, reset=init_ekf, temp_c=temp_c, charge_curr=sim.ib,
                               sat=is_sat(temp_c, mon.voc, mon.soc), t_last=mon.t_last)
            mon.calculate_charge_time(mon.q, mon.q_capacity, mon.ib, mon.soc)
            rp.delta_q, rp.t_last = mon.update()

            # Plot stuff
            current_in_s.append(current_in)
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
