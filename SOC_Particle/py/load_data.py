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
"""Utility to load data from csv files"""

import numpy as np
from DataOverModel import SavedData, SavedDataSim, write_clean_file
from CompareFault import add_stuff_f, filter_Tb, IB_BAND
from Battery import Battery, BatteryMonitor


# Load from files
def load_data(path_to_data, skip, unit_key, zero_zero_in, time_end_in, rated_batt_cap=Battery.UNIT_CAP_RATED,
              legacy=False, v1_only=False):

    print(f"load_data: {path_to_data=}\n{skip=}\n{unit_key=}\n{zero_zero_in=}\n{time_end_in=}\n{rated_batt_cap=}\n{legacy=}\n{v1_only=}")

    title_key = "unit,"  # Find one instance of title
    title_key_sel = "unit_s,"  # Find one instance of title
    unit_key_sel = "unit_sel"
    title_key_ekf = "unit_e,"  # Find one instance of title
    unit_key_ekf = "unit_ekf"
    title_key_sim = "unit_m,"  # Find one instance of title
    unit_key_sim = "unit_sim"
    temp_flt_file = 'flt_compareRunSim.txt'
    data_file_clean = write_clean_file(path_to_data, type_='_mon', title_key=title_key, unit_key=unit_key, skip=skip)
    if not legacy:
        cols = ('cTime', 'dt', 'chm', 'qcrs', 'sat', 'sel', 'mod', 'bmso', 'Tb', 'vb', 'ib', 'ib_charge', 'voc_soc',
                'vsat', 'dv_dyn', 'voc_stat', 'voc_ekf', 'y_ekf', 'soc_s', 'soc_ekf', 'soc')
    else:
        cols = ('cTime', 'dt', 'chm', 'sat', 'sel', 'mod', 'bmso', 'Tb', 'vb', 'ib', 'ib_charge', 'voc_soc',
                'vsat', 'dv_dyn', 'voc_stat', 'voc_ekf', 'y_ekf', 'soc_s', 'soc_ekf', 'soc')
    if data_file_clean is  not None:
        mon_raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols,  dtype=float, encoding=None).view(np.recarray)
    else:
        mon_raw = None
        print(f"load_data: returning mon=None")

    # Load sel (old)
    sel_file_clean = write_clean_file(path_to_data, type_='_sel', title_key=title_key_sel,
                                      unit_key=unit_key_sel, skip=skip)
    cols_sel = ('c_time', 'res', 'user_sel', 'cc_dif',
                'ibmh', 'ibnh', 'ibmm', 'ibnm', 'ibm', 'ib_diff', 'ib_diff_f',
                'voc_soc', 'e_w', 'e_w_f',
                'ib_sel_stat', 'ib_h', 'ib_s', 'mib', 'ib',
                'vb_sel', 'vb_h', 'vb_s', 'mvb', 'vb',
                'Tb_h', 'Tb_s', 'mtb', 'Tb_f',
                'fltw', 'falw', 'ib_rate', 'ib_quiet', 'tb_sel',
                'ccd_thr', 'ewh_thr', 'ewl_thr', 'ibd_thr', 'ibq_thr', 'preserving')
    if sel_file_clean and not v1_only:
        sel_raw = np.genfromtxt(sel_file_clean, delimiter=',', names=True, usecols=cols_sel, dtype=float, encoding=None).view(np.recarray)
    else:
        sel_raw = None
        print(f"load_data: returning sel_raw=None")

    # Load ekf (old)
    ekf_file_clean = write_clean_file(path_to_data, type_='_ekf', title_key=title_key_ekf,
                                      unit_key=unit_key_ekf, skip=skip)

    cols_ekf = ('c_time', 'Fx_', 'Bu_', 'Q_', 'R_', 'P_', 'S_', 'K_', 'u_', 'x_', 'y_', 'z_', 'x_prior_',
                'P_prior_', 'x_post_', 'P_post_', 'hx_', 'H_')
    if ekf_file_clean and not v1_only:
        ekf_raw = np.genfromtxt(ekf_file_clean, delimiter=',', names=True, usecols=cols_ekf, dtype=float, encoding=None).view(np.recarray)
    else:
        ekf_raw = None
        print(f"load_data: returning ekf_raw=None")

    mon = SavedData(data=mon_raw, sel=sel_raw, ekf=ekf_raw, time_end=time_end_in, zero_zero=zero_zero_in)
    if mon.chm is not None:
        chm = mon.chm[0]
    if path_to_data.__contains__('bb'):
        chm = 0
    elif path_to_data.__contains__('ch'):
        chm = 1
    else:
        chm = None
    batt = BatteryMonitor(chm)

    # Load sim _s v24 portion of real-time run (old)
    data_file_sim_clean = write_clean_file(path_to_data, type_='_sim', title_key=title_key_sim,
                                           unit_key=unit_key_sim, skip=skip)
    if not legacy:
        cols_sim = ('c_time', 'chm_s', 'qcrs_s', 'bmso_s', 'Tb_s', 'Tbl_s', 'vsat_s', 'voc_stat_s', 'dv_dyn_s', 'vb_s', 'ib_s',
                    'ib_in_s', 'ib_charge_s', 'ioc_s', 'sat_s', 'dq_s', 'soc_s', 'reset_s')
    else:
        cols_sim = ('c_time', 'chm_s', 'bmso_s', 'Tb_s', 'Tbl_s', 'vsat_s', 'voc_stat_s', 'dv_dyn_s', 'vb_s', 'ib_s',
                    'ib_in_s', 'ib_charge_s', 'ioc_s', 'sat_s', 'dq_s', 'soc_s', 'reset_s')
    if data_file_sim_clean and not v1_only:
        sim_raw = np.genfromtxt(data_file_sim_clean, delimiter=',', names=True, usecols=cols_sim, dtype=float, encoding=None).view(np.recarray)
        sim = SavedDataSim(time_ref=mon.time_ref, data=sim_raw, time_end=time_end_in)
    else:
        sim = None
        print(f"load_data: returning sim=None")

    # Load fault
    temp_flt_file_clean = write_clean_file(path_to_data, type_='_flt', title_key='fltb',
                                           unit_key='unit_f', skip=skip, comment_str='---')
    cols_f = ('time_ux', 'Tb_h', 'vb_h', 'ibmh', 'ibnh', 'Tb', 'vb', 'ib', 'soc', 'soc_ekf', 'voc', 'voc_stat',
              'e_w_f', 'fltw', 'falw')
    if temp_flt_file_clean and not v1_only:
        f_raw = np.genfromtxt(temp_flt_file_clean, delimiter=',', names=True, usecols=cols_f, dtype=None, encoding=None).view(np.recarray)
    else:
        print("data from", temp_flt_file, "empty after loading")
        f_raw = None
    if f_raw is not None:
        f_raw = np.unique(f_raw)
        f = add_stuff_f(f_raw, batt, ib_band=IB_BAND)
        print("\nf:\n", f, "\n")
        f = filter_Tb(f, 20., batt, tb_band=100., rated_batt_cap=rated_batt_cap)
    else:
        f = None
        print(f"load_data: returning f=None")

    return mon, sim, f, data_file_clean, temp_flt_file_clean


if __name__ == '__main__':
    path_to_data='G:/My Drive/GitHubArchive/SOC_Particle/dataReduction\\g20231111\\rapidTweakRegression_pro1a_bb.csv'
    skip=1
    unit_key='g20231111_pro1a_bb'
    zero_zero_in=False
    time_end_in=None
    rated_batt_cap=108.4
    legacy=False
    v1_only=False
    load_data(path_to_data=path_to_data, skip=skip, unit_key=unit_key, zero_zero_in=zero_zero_in, time_end_in=time_end_in,
              rated_batt_cap=rated_batt_cap, legacy=legacy, v1_only=v1_only)
