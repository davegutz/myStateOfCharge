# Calibrate_exec:  process data files assuming NOM_BATT_CAP = 100 then produce final schedules
# Copyright (C) 2024 Dave Gutz
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

"""Executive function workflow to take some raw soc-voc data taken with assumed NOM_BATT_CAP = 100, plot it, allow user
to enter fitted values of raw data, extract actual NOM_BATT_CAP then form it into final schedules.
"""
from Battery import Battery, BatteryMonitor
from DataOverModel import write_clean_file
import numpy as np


def plot_all_raw():
    return


def main():

    data_files = [('G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_voc 11C.csv', 5),
                  ('G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_voc 25C.csv', 13)]
    chm = 'ch'
    unit = 'soc2p2'
    unit_key = unit + '_' + chm
    title_key = 'voc_soc'

    raw_files = []
    for (data_file, temp) in data_files:
        data_file_clean = write_clean_file(data_file, type_='', title_key=title_key, unit_key=unit_key)
        if data_file_clean is None:
            print(f"case {data_file=} {temp=} is dirty...skipping")
            continue
        else:
            raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
            raw_files.append((raw, temp))

    print(f"{raw_files=}")
    plot_all_raw()


if __name__ == '__main__':
    main()

