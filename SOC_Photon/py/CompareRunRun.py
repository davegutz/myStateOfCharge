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
from DataOverModel import SavedData, SavedDataSim, write_clean_file, overall
from CompareHist import add_stuff_f, over_fault, filter_Tb, X_SOC_MIN_BB, T_SOC_MIN_BB, IB_BAND, RATED_BATT_CAP
from pyDAGx import myTables

# Battleborn Bmon=0, Bsim=0
t_x_soc0 = [-0.15, 0.00, 0.05, 0.10, 0.14, 0.17, 0.20, 0.25, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 0.99, 0.995,
            1.00]
t_y_t0 = [5., 11.1, 20., 30., 40.]
t_voc0 = [4.00, 4.00, 9.00, 11.80, 12.50, 12.60, 12.67, 12.76, 12.82, 12.93, 12.98, 13.03, 13.07, 13.11, 13.17,
          13.22, 13.59, 14.45,
          4.00, 4.00, 10.00, 12.30, 12.60, 12.65, 12.71, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20,
          13.23, 13.60, 14.46,
          4.00, 12.22, 12.66, 12.74, 12.80, 12.85, 12.89, 12.95, 12.99, 13.05, 13.08, 13.13, 13.18, 13.21, 13.25,
          13.27, 13.72, 14.50,
          4.00, 4.00, 12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26,
          13.27, 13.72, 14.50,
          4.00, 4.00, 4.00, 4.00, 10.50, 11.93, 12.78, 12.83, 12.89, 12.97, 13.06, 13.10, 13.13, 13.16, 13.19,
          13.20, 13.72, 14.50]
x0 = np.array(t_x_soc0)
y0 = np.array(t_y_t0)
data_interp0 = np.array(t_voc0)
lut_voc = myTables.TableInterp2D(x0, y0, data_interp0)
t_x_soc_min = [5., 11.1, 20., 30., 40.]
t_soc_min = [0.07, 0.05, -0.05, 0.00, 0.20]
lut_soc_min = myTables.TableInterp1D(np.array(t_x_soc_min), np.array(t_soc_min))


# Load from files
def load_data(data_file_old_txt, skip, path_to_data, path_to_temp, unit_key, zero_zero_in, time_end_in):
    title_key = "unit,"  # Find one instance of title
    title_key_sel = "unit_s,"  # Find one instance of title
    unit_key_sel = "unit_sel"
    title_key_ekf = "unit_e,"  # Find one instance of title
    unit_key_ekf = "unit_ekf"
    title_key_sim = "unit_m,"  # Find one instance of title
    unit_key_sim = "unit_sim"
    temp_flt_file = 'flt_compareRunSim.txt'
    data_file_clean = write_clean_file(data_file_old_txt, type_='_mon', title_key=title_key, unit_key=unit_key,
                                       skip=skip, path_to_data=path_to_data, path_to_temp=path_to_temp)
    cols = ('cTime', 'dt', 'chm', 'sat', 'sel', 'mod', 'bmso', 'Tb', 'vb', 'ib', 'ib_charge', 'ioc', 'voc_soc',
            'vsat', 'dv_dyn', 'voc_stat', 'voc_ekf', 'y_ekf', 'soc_s', 'soc_ekf', 'soc')
    mon_raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols,  dtype=float,
                                encoding=None).view(np.recarray)

    # Load sel (old)
    sel_file_clean = write_clean_file(data_file_old_txt, type_='_sel', title_key=title_key_sel,
                                      unit_key=unit_key_sel, skip=skip, path_to_data=path_to_data,
                                      path_to_temp=path_to_temp)
    cols_sel = ('c_time', 'res', 'user_sel', 'cc_dif',
                'ibmh', 'ibnh', 'ibmm', 'ibnm', 'ibm', 'ib_diff', 'ib_diff_f',
                'voc_soc', 'e_w', 'e_w_f',
                'ib_sel_stat', 'ib_h', 'ib_s', 'mib', 'ib',
                'vb_sel', 'vb_h', 'vb_s', 'mvb', 'vb',
                'Tb_h', 'Tb_s', 'mtb', 'Tb_f',
                'fltw', 'falw', 'ib_rate', 'ib_quiet', 'tb_sel',
                'ccd_thr', 'ewh_thr', 'ewl_thr', 'ibd_thr', 'ibq_thr', 'preserving')
    if sel_file_clean:
        sel_raw = np.genfromtxt(sel_file_clean, delimiter=',', names=True, usecols=cols_sel, dtype=float,
                                    encoding=None).view(np.recarray)
    else:
        sel_raw = None

    # Load ekf (old)
    ekf_file_clean = write_clean_file(data_file_old_txt, type_='_ekf', title_key=title_key_ekf,
                                      unit_key=unit_key_ekf, skip=skip, path_to_data=path_to_data,
                                      path_to_temp=path_to_temp)
    cols_ekf = ('c_time', 'Fx_', 'Bu_', 'Q_', 'R_', 'P_', 'S_', 'K_', 'u_', 'x_', 'y_', 'z_', 'x_prior_',
                'P_prior_', 'x_post_', 'P_post_', 'hx_', 'H_')
    ekf_raw = None
    if ekf_file_clean:
        ekf_raw = np.genfromtxt(ekf_file_clean, delimiter=',', names=True, usecols=cols_ekf, dtype=float,
                                    encoding=None).view(np.recarray)

    mon = SavedData(data=mon_raw, sel=sel_raw, ekf=ekf_raw, time_end=time_end_in,
                        zero_zero=zero_zero_in)

    # Load sim _s v24 portion of real-time run (old)
    data_file_sim_clean = write_clean_file(data_file_old_txt, type_='_sim', title_key=title_key_sim,
                                           unit_key=unit_key_sim, skip=skip, path_to_data=path_to_data,
                                           path_to_temp=path_to_temp)
    cols_sim = ('c_time', 'chm_s', 'bmso_s', 'Tb_s', 'Tbl_s', 'vsat_s', 'voc_stat_s', 'dv_dyn_s', 'vb_s', 'ib_s',
                'ib_in_s', 'ib_charge_s', 'ioc_s', 'sat_s', 'dq_s', 'soc_s', 'reset_s')
    if data_file_sim_clean:
        sim_raw = np.genfromtxt(data_file_sim_clean, delimiter=',', names=True, usecols=cols_sim,
                                    dtype=float, encoding=None).view(np.recarray)
        sim = SavedDataSim(time_ref=mon.time_ref, data=sim_raw, time_end=time_end_in)
    else:
        sim = None

    # Load fault
    temp_flt_file_clean = write_clean_file(data_file_old_txt, type_='_flt', title_key='fltb', unit_key='unit_f',
                                           skip=skip, path_to_data=path_to_data, path_to_temp=path_to_temp,
                                           comment_str='---')
    cols_f = ('time', 'Tb_h', 'vb_h', 'ibah', 'ibnh', 'Tb', 'vb', 'ib', 'soc', 'soc_ekf', 'voc', 'voc_stat',
              'e_w_f', 'fltw', 'falw')
    if temp_flt_file_clean:
        f_raw = np.genfromtxt(temp_flt_file_clean, delimiter=',', names=True, usecols=cols_f, dtype=None,
                              encoding=None).view(np.recarray)
    else:
        print("data from", temp_flt_file, "empty after loading")
    if temp_flt_file_clean:
        f_raw = np.unique(f_raw)
        f = add_stuff_f(f_raw, voc_soc_tbl=lut_voc, soc_min_tbl=lut_soc_min, ib_band=IB_BAND)
        print("\nf:\n", f, "\n")
        f = filter_Tb(f, 20., tb_band=100., rated_batt_cap=RATED_BATT_CAP)
    else:
        f = None
    return mon, sim, f, data_file_clean, temp_flt_file_clean


if __name__ == '__main__':
    import sys
    from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
    import matplotlib.pyplot as plt
    plt.rcParams['axes.grid'] = True
    from datetime import datetime

    def main():
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")

        # Regression
        # keys = [('ampHiFail v20221028.txt', 'pro0p_2022'), ('ampHiFail v20221220.txt', 'pro0p_2022')]

        # Compare
        keys = [('ampHiFail v20221220.txt', 'pro0p'), ('ampHiFail v20230125.txt', 'pro0p')]
        # keys = [('ampHiFail v20221220.txt', 'pro0p'), ('ampHiFail vA20221220.txt', 'soc1a')]
        # keys = [('rapidTweakRegression v20221220.txt', 'pro0p_2022'), ('rapidTweakRegression vA20221220.txt', 'soc1a_2022')]
        # keys = [('offSitHysBms v20221220.txt', 'pro0p_2022'), ('offSitHysBms vA20221220.txt', 'soc1a_2022')]

        # Transient  inputs
        skip = 1
        zero_zero_in = False
        time_end_in = None
        plot_init_in = False
        long_term_in = False
        plot_overall_in = True
        pathToSavePdfTo = '../dataReduction/figures'
        path_to_data = '../dataReduction'
        path_to_temp = '../dataReduction/temp'

        # Regression suite
        data_file_old_txt = keys[0][0]
        unit_key_old = keys[0][1]
        data_file_new_txt = keys[1][0]
        unit_key_new = keys[1][1]

        # Load data
        mon_old, sim_old, f, data_file_clean, temp_flt_file_clean = \
            load_data(data_file_old_txt, skip, path_to_data, path_to_temp, unit_key_old, zero_zero_in, time_end_in)
        mon_new, sim_new, f_new, dummy1, dummy2 = \
            load_data(data_file_new_txt, skip, path_to_data, path_to_temp, unit_key_new, zero_zero_in, time_end_in)

        # Plots
        n_fig = 0
        fig_files = []
        data_root = data_file_clean.split('/')[-1].replace('.csv', '-')
        # filename = data_root + sys.argv[0].split('\\')[-1]
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        if temp_flt_file_clean and len(f.time) > 1:
            n_fig, fig_files = over_fault(f, filename, fig_files=fig_files, plot_title=plot_title, subtitle='faults',
                                          n_fig=n_fig, x_sch=X_SOC_MIN_BB, z_sch=T_SOC_MIN_BB, voc_reset=0.,
                                          long_term=long_term_in)
        if plot_overall_in:
            n_fig, fig_files = overall(mon_old, mon_new, sim_old, sim_new, sim_new, filename, fig_files,
                                       plot_title=plot_title, n_fig=n_fig, plot_init_in=plot_init_in)  # all over all
        precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=pathToSavePdfTo)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo=pathToSavePdfTo)
        cleanup_fig_files(fig_files)

        plt.show()

    main()
