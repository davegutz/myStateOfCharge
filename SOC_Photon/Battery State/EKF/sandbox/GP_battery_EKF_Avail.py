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
from BatteryOld import BatteryOld
from BatteryEKF import BatteryEKF
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
        time_end = 2500
        # time_end = 51
        temp_c = 25.

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
        v_std = 0.01  # (0.0-0.01) ------ noise
        i_std = 0.1  # (0.0-0.1) ------ noise
        soc_init = 0.5  # (1.0-->0.8)  ------  initialization artifacts only
        tau_ct = 0.2  # (0.1-->5.)  -------

        # Setup
        r_std = 0.1  # Kalman sensor uncertainty (0.1) belief in meas
        q_std = 0.001  # Process uncertainty (0.001) belief in state
        scale = model_bat_cap / Battery.RATED_BATT_CAP
        battery_model = BatteryOld(nom_bat_cap=model_bat_cap, true_bat_cap=model_bat_cap,
                                   temp_c=temp_c, tau_ct=tau_ct)
        sim = BatteryModel(temp_c=temp_c, tau_ct=tau_ct, scale=scale)
        battery_ekf = BatteryEKF(rsd=rsd, tau_sd=tau_sd, r0=r0, tau_ct=tau_ct,
                                 rct=rct, tau_dif=tau_dif, r_dif=r_dif, temp_c=temp_c)
        mon = Battery(r_sd=rsd, tau_sd=tau_sd, r0=r0, tau_ct=tau_ct, r_ct=rct, tau_dif=tau_dif,
                          r_dif=r_dif, temp_c=temp_c)
        battery_ekf.R = r_std**2
        battery_ekf.Q = q_std**2
        battery_ekf.P = 100

        solve_max_err = 1e-6  # Solver error tolerance, V (1e-6)
        solve_max_counts = 10  # Solver maximum number of steps (10)
        solve_max_step = 0.2  # Solver maximum step size, frac soc

        # Executive tasks
        t = np.arange(0, time_end + dt, dt)
        current_in_s = []
        ib = []
        v_oc_s = []
        vbs = []
        vcs = []
        vds = []
        i_oc_s = []
        i_ct_s = []
        i_r_ct_s = []
        i_c_dif_s = []
        i_r_dif_s = []
        v_bc_dot_s = []
        v_cd_dot_s = []
        soc_s = []
        soc_norm_s = []
        soc_avail_s = []
        pow_s = []
        v_batt_s = []
        i_batt_s = []
        soc_norm_ekf_s = []
        soc_avail_ekf_s = []
        voc_dyn_s = []
        soc_norm_filtered_s = []
        voc_filtered_s = []
        prior_soc_s = []
        x_s = []
        z_s = []
        k_s = []
        fx_s = []
        bu_s = []
        h_s = []
        hx_s = []
        y_s = []
        p_s = []
        e_soc_ekf_s = []
        e_voc_ekf_s = []
        e_soc_norm_ekf_s = []
        soc_solved_s = []
        vbat_solved_s = []
        e_soc_solved_ekf_s = []

        for i in range(len(t)):
            if t[i] < 50:
                current_in = 0.
            elif t[i] < 450:
                current_in = -40.
            elif t[i] < 1000:
                current_in = 0.
            elif t[i] < 1400:
                current_in = 40.
            else:
                current_in = 0.
            init_ekf = (t[i] <= 1)

            if init_ekf:
                sim.apply_soc(soc_init, temp_c)
                rp.delta_q_model = sim.delta_q
                rp.t_last_model = temp_c+dt_model
                sim.load(rp.delta_q_model, rp.t_last_model)
                sim.init_battery()
                sim.apply_delta_q_t(rp.delta_q_model, rp.t_last_model)

            # Models
            battery_model.calc_voc(temp_c=temp_c+dt_model, soc_init=soc_init)
            u = np.array([current_in, battery_model.voc]).T
            battery_model.calc_dynamics(u, dt=dt, i_hyst=i_hyst, temp_c=temp_c)
            sim.calculate(temp_c=temp_c, soc=soc_init, curr_in=current_in, dt=dt, q_capacity=sim.q_capacity)
            sim.count_coulombs(dt=dt, reset=init_ekf, temp_c=temp_c, charge_curr=current_in, t_last=rp.t_last_model)
            rp.delta_q_model, rp.t_last_model = sim.update()


            # EKF
            if init_ekf:
                battery_ekf.assign_temp_c(temp_c)
                battery_ekf.assign_soc_norm(float(battery_model.soc_norm), battery_model.voc)
                battery_ekf.x_kf = battery_model.soc_norm + soc_init_err

                mon.apply_soc(soc_init, temp_c)
                rp.delta_q = mon.delta_q
                rp.t_last = temp_c
                mon.load(rp.delta_q, rp.t_last)
                mon.assign_temp_c(temp_c)
                mon.init_battery()
                mon.init_soc_ekf(soc_init)  # when modeling (assumed in python) ekf wants to equal model

            # Setup
            u_dyn = np.array([battery_model.i_batt + randn()*i_std + di_sense,
                              battery_model.v_batt + randn()*v_std + dv_sense]).T
            battery_ekf.calc_dynamics_ekf(u_dyn, dt=dt_ekf)
            battery_ekf.coulomb_counter_ekf()
            battery_ekf.coulomb_counter_avail(temp_c)

            # Monitor calculations including ekf
            mon.calculate_ekf(temp_c, sim.vb+randn()*v_std+dv_sense, sim.ib+randn()*i_std+di_sense, dt_ekf)
            mon.count_coulombs(dt=dt_ekf, reset=init_ekf, temp_c=temp_c, charge_curr=current_in,
                                   sat=is_sat(temp_c, mon.voc), t_last=mon.t_last)
            mon.calculate_charge_time(mon.q, mon.q_capacity, current_in, mon.soc)
            rp.delta_q, rp.t_last = mon.update()

            # Call Kalman Filters
            battery_ekf.kf_predict_1x1(u=battery_ekf.ib)
            battery_ekf.kf_update_1x1(battery_ekf.voc_dyn)

            # if t[i] < 1.:
            #     print("soc= %7.3f, %7.3f, %7.3f, %7.3f,    vb= %7.3f, %7.3f, %7.3f, %7.3f    ib= %7.3f, %7.3f    voc= %7.3f, %7.3f"
            #           % (battery_ekf.soc, mon.soc, battery_model.soc, sim.soc,
            #              battery_ekf.vb, mon.vb, battery_model.vb, sim.vb,
            #              battery_ekf.ib, mon.ib, battery_ekf.voc, mon.voc))

            # Solver (does same thing as EKF, noisier)
            if True:
                vbat_f_o = battery_ekf.voc_dyn
                count = 0
                soc_solved = battery_ekf.soc
                dv_dsoc = battery_ekf.dV_dSoc_norm
                vbat_solved = battery_ekf.calc_voc_solve(soc_solved, dv_dsoc)
                solve_err = vbat_f_o - vbat_solved
                while abs(solve_err) > solve_max_err and count < solve_max_counts:
                    count += 1
                    soc_solved = max(min(soc_solved + max(min(solve_err/dv_dsoc, solve_max_step), -solve_max_step),
                                         battery_ekf.eps_max), battery_ekf.eps_min)
                    vbat_solved = battery_ekf.calc_voc_solve(soc_solved, dv_dsoc)
                    solve_err = vbat_f_o - vbat_solved

            # Plot stuff
            e_soc_ekf = (battery_ekf.soc_norm_filtered - battery_model.soc_norm) / battery_model.soc_norm
            e_voc_ekf = (battery_ekf.voc_filtered - battery_model.voc) / battery_model.voc
            e_soc_norm_ekf = (battery_ekf.soc_norm - battery_model.soc_norm) / battery_model.soc_norm
            e_soc_solved_ekf = (soc_solved - battery_model.soc_norm) / battery_model.soc_norm

            current_in_s.append(current_in)
            ib.append(battery_model.ib)
            v_oc_s.append(battery_model.voc)
            vbs.append(battery_model.vb)
            vcs.append(battery_model.vc())
            vds.append(battery_model.vd())
            i_oc_s.append(battery_model.ioc)
            i_ct_s.append(battery_model.i_c_ct())
            i_c_dif_s.append(battery_model.i_c_dif())
            i_r_ct_s.append(battery_model.i_r_ct())
            i_r_dif_s.append(battery_model.i_r_dif())
            v_bc_dot_s.append(battery_model.vbc_dot())
            v_cd_dot_s.append(battery_model.vcd_dot())
            soc_norm_s.append(battery_model.soc_norm)
            soc_avail_s.append(battery_model.soc_avail)
            soc_s.append(battery_model.soc)
            pow_s.append(battery_model.pow_in)
            soc_norm_ekf_s.append(battery_ekf.soc_norm)
            soc_avail_ekf_s.append(battery_ekf.soc_avail)
            voc_dyn_s.append(battery_ekf.voc_dyn)
            v_batt_s.append(battery_model.v_batt)
            i_batt_s.append(battery_model.i_batt)
            soc_norm_filtered_s.append(battery_ekf.soc_norm_filtered)
            voc_filtered_s.append(battery_ekf.voc_filtered)
            prior_soc_s.append(battery_ekf.x_prior)
            x_s.append(battery_ekf.x_kf)
            z_s.append(battery_ekf.z_ekf)
            k_s.append(float(battery_ekf.K))
            fx_s.append(battery_ekf.Fx)
            bu_s.append(battery_ekf.Bu)
            h_s.append(float(battery_ekf.H))
            hx_s.append(float(battery_ekf.hx))
            y_s.append(float(battery_ekf.y_kf))
            p_s.append(float(battery_ekf.P))
            soc_solved_s.append(soc_solved)
            vbat_solved_s.append(vbat_solved)
            e_soc_ekf_s.append(e_soc_ekf)
            e_voc_ekf_s.append(e_voc_ekf)
            e_soc_norm_ekf_s.append(e_soc_norm_ekf)
            e_soc_solved_ekf_s.append(e_soc_solved_ekf)

            mon.save(t[i], sim.soc, sim.voc)

        # Data
        print('mon:  ', str(mon))
        print('sim:  ', str(sim))

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time

        plt.figure()
        n_fig += 1
        plt.subplot(321)
        plt.title(plot_title)
        plt.plot(t, current_in_s, color='black', label='curr dmd, A')
        plt.plot(t, ib, color='green', label='ib')
        plt.plot(t, i_r_ct_s, color='red', label='I_R_ct')
        plt.plot(t, i_c_dif_s, color='cyan', label='I_C_dif')
        plt.plot(t, i_r_dif_s, color='orange', linestyle='--', label='I_R_dif')
        plt.plot(t, i_oc_s, color='black', linestyle='--', label='Ioc')
        plt.legend(loc=1)
        plt.subplot(323)
        plt.plot(t, vbs, color='green', label='Vb')
        plt.plot(t, vcs, color='blue', label='Vc')
        plt.plot(t, vds, color='red', label='Vd')
        plt.plot(t, v_oc_s, color='orange', label='Voc')
        plt.legend(loc=1)
        plt.subplot(325)
        plt.plot(t, v_bc_dot_s, color='green', label='Vbc_dot')
        plt.plot(t, v_cd_dot_s, color='blue', label='Vcd_dot')
        plt.legend(loc=1)
        plt.subplot(322)
        plt.plot(t, soc_s, color='green', label='SOC')
        plt.plot(t, soc_norm_s, color='red', label='SOC_norm')
        plt.legend(loc=1)
        plt.subplot(324)
        plt.plot(t, pow_s, color='orange', label='Pow_in')
        plt.legend(loc=1)
        plt.subplot(326)
        plt.plot(soc_norm_s, v_oc_s, color='black', label='voc vs soc_norm')
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()
        n_fig += 1
        plt.subplot(321)
        plt.title(plot_title)
        plt.plot(t, ib, color='black', label='ib')
        plt.plot(t, i_batt_s, color='magenta', linestyle='dotted', label='i_batt')
        plt.legend(loc=3)
        plt.subplot(322)
        plt.plot(t, soc_norm_s, color='red', label='SOC_norm')
        plt.plot(t, soc_norm_ekf_s, color='black', linestyle='dotted', label='SOC_norm_ekf')
        plt.ylim(0.92, 1.01)
        plt.legend(loc=4)
        plt.subplot(323)
        plt.plot(t, v_oc_s, color='blue', label='actual voc')
        plt.plot(t, voc_dyn_s, color='red', linestyle='dotted', label='voc_dyn meas')
        plt.plot(t, voc_filtered_s, color='green', label='voc_filtered state')
        plt.plot(t, vbs, color='black', label='vb')
        plt.plot(t, v_batt_s, color='magenta', linestyle='dotted', label='v_batt')
        plt.ylim(11, 16)
        plt.legend(loc=4)
        plt.subplot(324)
        plt.plot(t, soc_norm_s, color='black', linestyle='dotted', label='SOC_norm')
        plt.plot(t, prior_soc_s, color='red', linestyle='dotted', label='post soc_norm_filtered')
        plt.plot(t, x_s, color='green', label='x soc_norm_filtered')
        plt.ylim(0.92, 1.01)
        plt.legend(loc=4)
        plt.subplot(325)
        plt.plot(t, k_s, color='red', linestyle='dotted', label='K (belief state / belief meas)')
        plt.legend(loc=4)
        plt.subplot(326)
        plt.plot(t, e_soc_ekf_s, color='red', linestyle='dotted', label='e_soc_ekf')
        plt.plot(t, e_voc_ekf_s, color='blue', linestyle='dotted', label='e_voc')
        plt.plot(t, e_soc_norm_ekf_s, color='black', linestyle='dotted', label='e_soc_norm to User')
        plt.ylim(-0.01, 0.01)
        plt.legend(loc=2)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        # plt.figure()
        # n_fig += 1
        # plt.subplot(331)
        # plt.title(plot_title)
        # plt.plot(t, x_s, color='red', linestyle='dotted', label='x ekf')
        # plt.legend(loc=4)
        # plt.subplot(332)
        # plt.plot(t, hx_s, color='cyan', linestyle='dotted', label='hx ekf')
        # plt.plot(t, z_s, color='black', linestyle='dotted', label='z ekf')
        # plt.legend(loc=4)
        # plt.subplot(333)
        # plt.plot(t, y_s, color='green', linestyle='dotted', label='y ekf')
        # plt.legend(loc=4)
        # plt.subplot(334)
        # plt.plot(t, h_s, color='magenta', linestyle='dotted', label='H ekf')
        # plt.ylim(0, 50)
        # plt.legend(loc=3)
        # plt.subplot(335)
        # plt.plot(t, p_s, color='orange', linestyle='dotted', label='P ekf')
        # plt.legend(loc=3)
        # plt.subplot(336)
        # plt.plot(t, fx_s, color='red', linestyle='dotted', label='Fx ekf')
        # plt.legend(loc=2)
        # plt.subplot(337)
        # plt.plot(t, bu_s, color='blue', linestyle='dotted', label='Bu ekf')
        # plt.legend(loc=2)
        # plt.subplot(338)
        # plt.plot(t, k_s, color='red', linestyle='dotted', label='K ekf')
        # plt.legend(loc=4)
        # fig_file_name = filename + '_' + str(n_fig) + ".png"
        # fig_files.append(fig_file_name)
        # plt.savefig(fig_file_name, format="png")
        #
        # plt.figure()
        # n_fig += 1
        # plt.subplot(121)
        # plt.title(plot_title)
        # plt.plot(t, voc_dyn_s, color='black', label='voc_dyn')
        # plt.plot(t, vbat_solved_s, color='green', linestyle='dotted', label='vbat_solved')
        # plt.legend(loc=4)
        # plt.subplot(122)
        # plt.plot(t, soc_s, color='black', label='soc')
        # plt.plot(t, soc_solved_s, color='green', linestyle='dotted', label='soc_solved')
        # plt.legend(loc=4)
        # fig_file_name = filename + '_' + str(n_fig) + ".png"
        # fig_files.append(fig_file_name)
        # plt.savefig(fig_file_name, format="png")
        #
        # plt.figure()
        # n_fig += 1
        # plt.title(plot_title)
        # plt.plot(t, e_voc_ekf_s, color='blue', linestyle='dotted', label='e_voc')
        # plt.plot(t, e_soc_solved_ekf_s, color='green', linestyle='dotted', label='e_soc_norm to User')
        # plt.plot(t, e_soc_ekf_s, color='red', linestyle='dotted', label='e_soc_ekf')
        # plt.plot(t, e_soc_norm_ekf_s, color='cyan', linestyle='dotted', label='e_soc_norm to User')
        # plt.ylim(-0.01, 0.01)
        # plt.legend(loc=2)
        # fig_file_name = filename + '_' + str(n_fig) + ".png"
        # fig_files.append(fig_file_name)
        # plt.savefig(fig_file_name, format="png")
        #
        # plt.figure()
        # n_fig += 1
        # plt.title(plot_title)
        # plt.plot(t, soc_avail_s, color='black', linestyle='dotted', label='soc_avail')
        # plt.plot(t, soc_avail_ekf_s, color='blue', linestyle='dotted', label='soc_avail_ekf')
        # plt.legend(loc=4)
        # fig_file_name = filename + '_' + str(n_fig) + ".png"
        # fig_files.append(fig_file_name)
        # plt.savefig(fig_file_name, format="png")
        #
        # plt.figure()
        # n_fig += 1
        # plt.title(plot_title)
        # plt.plot(t, e_voc_ekf_s, color='blue', linestyle='dotted', label='e_voc')
        # plt.plot(t, e_soc_solved_ekf_s, color='green', linestyle='dotted', label='e_soc_norm to User')
        # plt.plot(t, e_soc_ekf_s, color='red', linestyle='dotted', label='e_soc_ekf')
        # plt.plot(t, e_soc_norm_ekf_s, color='cyan', linestyle='dotted', label='e_soc_norm to User')
        # plt.legend(loc=2)
        # fig_file_name = filename + '_' + str(n_fig) + ".png"
        # fig_files.append(fig_file_name)
        # plt.savefig(fig_file_name, format="png")

        n_fig, fig_files = overall(mon.saved, sim.saved, filename, fig_files,
                                           plot_title=plot_title, n_fig = n_fig, ref=current_in_s)

        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='figures')
        for fig_file in fig_files:
            try:
                os.remove(fig_file)
            except OSError:
                pass
        plt.show()

    main()
