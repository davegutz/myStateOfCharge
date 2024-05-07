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
from resample import remove_nan


def find_sync(path_to_data):
    sync = []
    with open(path_to_data) as in_file:
        for line in in_file:
            if line.__contains__('SYNC'):
                sync.append(float(line.strip().split(',')[-1]))

    if not sync:
        sync = None
    else:
        sync = np.array(sync)
    return sync


def calculate_master_sync(ref, test):
    delta = np.maximum(ref, test)
    return delta


class SyncInfo:
    """Shift time arrays to synchronize two different data sets for CompareRunRun usage"""
    def __init__(self, sav_mon, sav_sim=None, sync=None):
        self.is_empty = False
        if sync is None or sav_mon is None:
            self.is_empty = True
            return
        self.time_mon = sav_mon.time  # pointer
        self.time_sim = sav_sim.time  # pointer
        self.sync_cTime = sync
        self.cTime = sav_mon.cTime
        self.time = sav_mon.time
        self.int_mon = []
        self.int_sim = []
        self.length = len(sync)
        rel = []
        delta = []
        for i in np.arange(self.length):
            rel.append(self.sync_cTime[i] - sav_mon.cTime[0])
            if i == 0:
                delta.append(rel[0])
                self.int_mon.append([np.where(sav_mon.cTime <= sync[i])])
                self.int_sim.append([np.where(sav_sim.cTime <= sync[i])])
            else:
                delta.append(rel[i] - rel[i-1])
                self.int_mon.append([np.where((sav_mon.cTime <= sync[i]) & (sav_mon.cTime > sync[i-1]))])
                self.int_sim.append([np.where((sav_sim.cTime <= sync[i]) & (sav_sim.cTime > sync[i - 1]))])
        self.int_mon.append([np.where(sav_mon.cTime > sync[self.length-1])])
        self.int_sim.append([np.where(sav_sim.cTime > sync[self.length-1])])
        self.rel_mon = np.array(rel)
        self.del_mon = np.array(delta)
        return

    def synchronize(self, sync_del):
        """Call this after building two class instances and calling calculate_master_sync to make sync_del"""
        # Init entire time array again.  First sync is always 0
        orig = self.time_mon.copy()
        acc_shift = self.sync_cTime[0]
        self.time_mon = self.cTime.copy()
        self.time_sim = self.cTime.copy()

        # Subsequent sets based on difference to master del
        n = 0
        for i in np.arange(self.length+1):
            if 0 < i < self.length:
                acc_shift -= sync_del[i] - self.del_mon[i]
            self.time_mon[self.int_mon[i]] = (self.time_mon[self.int_mon[i]] - acc_shift).copy()
            self.time_sim[self.int_sim[i]] = (self.time_sim[self.int_sim[i]] - acc_shift).copy()
            n += 1
            print(f"{i=}: {self.time_mon[self.int_mon[i]]=}")

        print(f"{self.time_mon=}")
        return


# Load from files
def load_data(path_to_data, skip, unit_key, zero_zero_in, time_end_in, rated_batt_cap=Battery.UNIT_CAP_RATED,
              legacy=False, v1_only=False, zero_thr_in=0.02):

    print(f"load_data: \n{path_to_data=}\n{skip=}\n{unit_key=}\n{zero_zero_in=}\n{time_end_in=}\n{rated_batt_cap=}\n"
          f"{legacy=}\n{v1_only=}")

    hdr_key = "unit,"  # Find one instance of title
    hdr_key_sel = "unit_s,"  # Find one instance of title
    unit_key_sel = "unit_sel"
    hdr_key_ekf = "unit_e,"  # Find one instance of title
    unit_key_ekf = "unit_ekf"
    hdr_key_sim = "unit_m,"  # Find one instance of title
    unit_key_sim = "unit_sim"
    temp_flt_file = 'flt_compareRunSim.txt'

    sync = find_sync(path_to_data)

    data_file_clean = write_clean_file(path_to_data, type_='_mon', hdr_key=hdr_key, unit_key=unit_key, skip=skip)
    if data_file_clean is None:
        return None, None, None, None, None
    if data_file_clean is not None:
        mon_raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
    else:
        mon_raw = None
        print(f"load_data: returning mon=None")

    # Load sel (old)
    sel_file_clean = write_clean_file(path_to_data, type_='_sel', hdr_key=hdr_key_sel,
                                      unit_key=unit_key_sel, skip=skip)
    if sel_file_clean and not v1_only:
        sel_raw = np.genfromtxt(sel_file_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
    else:
        sel_raw = None
        print(f"load_data: returning sel_raw=None")

    # Load ekf (old)
    ekf_file_clean = write_clean_file(path_to_data, type_='_ekf', hdr_key=hdr_key_ekf,
                                      unit_key=unit_key_ekf, skip=skip)
    if ekf_file_clean and not v1_only:
        ekf_raw = np.genfromtxt(ekf_file_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
    else:
        ekf_raw = None
        print(f"load_data: returning ekf_raw=None")

    mon = SavedData(data=mon_raw, sel=sel_raw, ekf=ekf_raw, time_end=time_end_in, zero_zero=zero_zero_in,
                    zero_thr=zero_thr_in, sync_cTime=sync)
    if mon.chm is not None:
        chm = int(mon.chm[-1])
    elif path_to_data.__contains__('bb'):
        chm = 0
    elif path_to_data.__contains__('ch'):
        chm = 1
    else:
        chm = None
    batt = BatteryMonitor(chm)

    # Load sim _s v24 portion of real-time run (old)
    data_file_sim_clean = write_clean_file(path_to_data, type_='_sim', hdr_key=hdr_key_sim,
                                           unit_key=unit_key_sim, skip=skip)
    if data_file_sim_clean and not v1_only:
        sim_raw = np.genfromtxt(data_file_sim_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
        sim = SavedDataSim(time_ref=mon.time_ref, data=sim_raw, time_end=time_end_in)
    else:
        sim = None
        print(f"load_data: returning sim=None")

    # Calculate sync information
    sync_info = SyncInfo(sav_mon=mon, sav_sim=sim, sync=sync)

    # Load fault
    temp_flt_file_clean = write_clean_file(path_to_data, type_='_flt', hdr_key='fltb',
                                           unit_key='unit_f', skip=skip, comment_str='---')
    if temp_flt_file_clean and not v1_only:
        f_raw = np.genfromtxt(temp_flt_file_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
    else:
        print("data from", temp_flt_file, "empty after loading")
        f_raw = None
    if f_raw is not None:
        f_raw = np.unique(f_raw)
        f_raw = remove_nan(f_raw)
        f = add_stuff_f(f_raw, batt, ib_band=IB_BAND)
        print("\nload_data:  f:\n", f, "\n")
        f = filter_Tb(f, 20., batt, tb_band=100., rated_batt_cap=rated_batt_cap)
    else:
        f = None
        print(f"load_data: returning f=None")

    return mon, sim, f, data_file_clean, temp_flt_file_clean, sync_info


def main():
    path_to_data = '/home/daveg/google-drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/allInCHG_pro0p_chg.csv'
    skip = 1
    unit_key = 'g20240331_pro0p_chg '
    zero_zero_in = False
    time_end_in = None
    rated_batt_cap = 108.4
    legacy = False
    v1_only = False
    load_data(path_to_data=path_to_data, skip=skip, unit_key=unit_key, zero_zero_in=zero_zero_in,
              time_end_in=time_end_in, rated_batt_cap=rated_batt_cap, legacy=legacy, v1_only=v1_only)


if __name__ == '__main__':
    main()
