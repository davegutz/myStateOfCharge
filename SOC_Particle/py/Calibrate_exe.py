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
    def __init__(self, nom_unit_cap=1., temp=25., data=None, data_file=''):
        self.nom_unit_cap = nom_unit_cap
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
        s = 'nom_unit_cap={:7.3f}\ntemp={:7.3f}\ndata_file={:s}\n soc, voc_soc,\n'.format(self.nom_unit_cap, self.temp_c, self.data_file)
        for i in np.arange(len(self.soc)):
            s += "{:7.3f}, {:7.3f},\n".format(self.soc[i], self.voc_soc[i])
        return s


class DataCP(DataC):
    def __init__(self, nom_unit_cap, temp, xdata=None, ydata=None, data_file=''):
        DataC.__init__(self, nom_unit_cap, temp, None, data_file=data_file)
        if xdata is None or ydata is None:
            self.soc = None
            self.voc_soc = None
        else:
            self.i = 0
            self.soc = np.array(xdata)
            self.voc_soc = np.array(ydata)


def mash(raw_data):
    """Mash all data together normalized with same ordinates"""
    soc = []
    for (item, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in raw_data:
        for soc_data in item.soc:
            soc_normal = normalize_soc(soc_data, nom_unit_cap, Battery.UNIT_CAP_RATED)
            soc.append(soc_normal)
    soc_sort = np.unique(np.array(soc))
    mashed_data = []
    for (item, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in raw_data:
        soc_normal = normalize_soc(item.soc, nom_unit_cap, Battery.UNIT_CAP_RATED)
        lut_voc_soc = myTables.TableInterp1D(soc_normal, item.voc_soc)
        voc_soc_sort = []
        for soc in soc_sort:
            voc_soc_sort.append(lut_voc_soc.interp(soc))
        New_raw_data = DataCP(nom_unit_cap, item.temp_c, soc_sort, voc_soc_sort, item.data_file)
        mashed_data.append((New_raw_data, Battery.UNIT_CAP_RATED, temp, p_color, p_style, marker, marker_size))
    return mashed_data


def normalize_soc(soc_data, nom_unit_cap_data, unit_cap_rated=100.):
    """Normalize for saturation defined to be soc = 1"""
    soc_normal = 1. - (1. - soc_data) * nom_unit_cap_data / unit_cap_rated
    return soc_normal


def plot_all_raw(raw_data, mashed_data, fig_files=None, plot_title=None, fig_list=None, filename='Calibrate_exe'):
    """Plot the various stages of data reduction"""
    if fig_files is None:
        fig_files = []

    fig_list.append(plt.figure())  # raw data 1
    plt.subplot(111)
    plt.title(plot_title + ' raw')
    for (item, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in raw_data:
        plt.plot(item.soc, item.voc_soc, color=p_color, linestyle=p_style, marker=marker, markersize=marker_size,
                 label='vstat ' + str(nom_unit_cap) + '  ' + str(temp) + 'C')
    plt.legend(loc=1)
    plt.xlabel('soc')
    plt.ylabel('voc unit, v')
    plt.grid()

    fig_list.append(plt.figure())  # raw data 2
    plt.subplot(111)
    plt.title(plot_title + ' mashed')
    for (item, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in mashed_data:
        plt.plot(item.soc, item.voc_soc, color=p_color, linestyle=p_style, marker=marker, markersize=marker_size,
                 label='vstat ' + str(nom_unit_cap) + '  ' + str(temp) + 'C')
    plt.legend(loc=1)
    plt.xlabel('soc normal')
    plt.ylabel('voc unit, v')
    plt.grid()

    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return fig_list, fig_files


def main():

    data_files = [('G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_voc 11C soc2p2_ch.csv', 100., 11., 'blue', '-.', 'o', 4),
                  ('G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_voc 21p5C soc2p2_ch.csv', 100., 21.5, 'orange', '--', 'x', 4),
                  ('G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_voc 25C soc2p2_ch.csv', 110., 25., 'red', '-', '1', 4)]
    #  data_files[( data_file, nom_unit_cap, temp_c, p_color, marker, marker_size), (...), ...]  List of tuples of data from experiments
    #   data_file       Full path to data file
    #   nom_unit_cap    Nominal applied battery unit (100 Ah unit) capacity, Ah
    #   temp_c          Temperature bulk, Tb, in deg C
    #   p_color         matplotlib line color
    #   marker          matplotlib marker
    #   marker_size     matplotlib marker size
    chm = 'ch'  # battery chemistry
    unit = 'soc2p2'  # Particle unit name
    unit_key = unit + '_' + chm  # key to be found in each line of data
    title_key = 'voc_soc'  # key to be found in header for data

    # Initialize
    raw_files = []
    Raw_files = []
    fig_list = []
    fig_files = []
    data_file_clean = None
    for (data_file, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in data_files:
        data_file_clean = write_clean_file(data_file, type_='', title_key=title_key, unit_key=unit_key)
        if data_file_clean is None:
            print(f"case {data_file=} {temp=} is dirty...skipping")
            continue
        else:
            raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
            Raw_data = DataC(nom_unit_cap, temp, raw, data_file_clean)
            raw_files.append((raw, nom_unit_cap, temp))
            Raw_files.append((Raw_data, nom_unit_cap, temp, p_color, p_style, marker, marker_size))

    # Mash all the data to one table and normalize to NOM_UNIT_CAP = 100
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
