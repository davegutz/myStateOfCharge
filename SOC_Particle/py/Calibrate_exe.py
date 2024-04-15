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
from unite_pictures import cleanup_fig_files
from DataOverModel import write_clean_file
import matplotlib.pyplot as plt
from datetime import datetime
from pyDAGx import myTables
import numpy as np
import sys


class DataC:
    def __init__(self, temp, data=None, data_file=''):
        self.temp_c = temp
        self.data_file = data_file
        if data is None:
            self.soc = None
            self.voc_soc = None
        else:
            self.i = 0
            self.soc = np.array(data.soc)
            self.voc_soc = np.array(data.voc_soc)

    def __str__(self):
        s = 'temp={:7.3f}\ndata_file={:s}\n soc, voc_soc,\n'.format(self.temp_c, self.data_file)
        for i in np.arange(len(self.soc)):
            s += "{:7.3f}, {:7.3f},\n".format(self.soc[i], self.voc_soc[i])
        return s


class DataCP(DataC):
    def __init__(self, temp, xdata=None, ydata=None, data_file=''):
        DataC.__init__(self, temp, None, data_file=data_file)
        if xdata is None or ydata is None:
            self.soc = None
            self.voc_soc = None
        else:
            self.i = 0
            self.soc = np.array(xdata)
            self.voc_soc = np.array(ydata)


def mash(raw_data):
    soc = []
    for (item, temp, p_color, p_style, marker, marker_size) in raw_data:
        for soc_data in item.soc:
            soc.append(soc_data)
    soc_sort = np.unique(np.array(soc))
    mashed_data = []
    for (item, temp, p_color, p_style, marker, marker_size) in raw_data:
        lut_voc_soc = myTables.TableInterp1D(item.soc, item.voc_soc)
        voc_soc_sort = []
        for soc in soc_sort:
            voc_soc_sort.append(lut_voc_soc.interp(soc))
        New_raw_data = DataCP(item.temp_c, soc_sort, voc_soc_sort, item.data_file)
        mashed_data.append((New_raw_data, temp, p_color, p_style, marker, marker_size))
    return mashed_data


def plot_all_raw(raw_data, mashed_data, fig_files=None, plot_title=None, fig_list=None, filename='Calibrate_exe'):
    if fig_files is None:
        fig_files = []

    fig_list.append(plt.figure())  # raw data 1
    plt.subplot(111)
    plt.title(plot_title + ' raw')
    for (item, temp, p_color, p_style, marker, marker_size) in raw_data:
        plt.plot(item.soc, item.voc_soc, color=p_color, linestyle=p_style, marker=marker, markersize=marker_size, label='voc_soc ' + str(temp) + 'C')
    plt.legend(loc=1)
    plt.grid()

    fig_list.append(plt.figure())  # raw data 2
    plt.subplot(111)
    plt.title(plot_title + ' mashed')
    for (item, temp, p_color, p_style, marker, marker_size) in mashed_data:
        plt.plot(item.soc, item.voc_soc, color=p_color, linestyle=p_style, marker=marker, markersize=marker_size, label='voc_soc ' + str(temp) + 'C')
    plt.legend(loc=1)
    plt.grid()

    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return fig_list, fig_files


def main():

    data_files = [('G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_voc 11C soc2p2_ch.csv', 11, 'blue', '-.', 'o', 4),
                  ('G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_voc 21p5C soc2p2_ch.csv', 21.5, 'orange', '-', 'x', 4)]
    chm = 'ch'
    unit = 'soc2p2'
    unit_key = unit + '_' + chm
    title_key = 'voc_soc'

    raw_files = []
    Raw_files = []
    fig_list = []
    fig_files = []
    data_file_clean = None
    for (data_file, temp, p_color, p_style, marker, marker_size) in data_files:
        data_file_clean = write_clean_file(data_file, type_='', title_key=title_key, unit_key=unit_key)
        if data_file_clean is None:
            print(f"case {data_file=} {temp=} is dirty...skipping")
            continue
        else:
            raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
            Raw_data = DataC(temp, raw, data_file_clean)
            # print(Raw)
            raw_files.append((raw, temp))
            Raw_files.append((Raw_data, temp, p_color, p_style, marker, marker_size))
    Mashed_data = mash(Raw_files)

    date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
    data_root = data_file_clean.split('/')[-1].replace('.csv', '_')
    filename = data_root + sys.argv[0].split('/')[-1].split('\\')[-1].split('.')[-2]
    plot_title = filename + '   ' + date_time
    plot_all_raw(Raw_files, Mashed_data, fig_files=fig_files, plot_title=plot_title, fig_list=fig_list, filename='Calibrate_exe')
    plt.show()
    cleanup_fig_files(fig_files)


if __name__ == '__main__':
    main()
