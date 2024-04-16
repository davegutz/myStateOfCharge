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
from Battery import Battery
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
            self.vstat = None
        else:
            self.i = 0
            self.soc = np.array(data.soc)
            self.vstat = np.array(data.vstat)

    def __str__(self):
        s = 'nom_unit_cap={:7.3f}\ntemp={:7.3f}\ndata_file={:s}\n soc, voc_soc,\n'.format(self.nom_unit_cap, self.temp_c, self.data_file)
        for i in np.arange(len(self.soc)):
            s += "{:7.3f}, {:7.3f},\n".format(self.soc[i], self.vstat[i])
        return s


class DataCP(DataC):
    def __init__(self, nom_unit_cap, temp, xdata=None, ydata=None, data_file=''):
        DataC.__init__(self, nom_unit_cap, temp, None, data_file=data_file)
        if xdata is None or ydata is None:
            self.soc = None
            self.vstat = None
        else:
            self.i = 0
            self.soc = np.array(xdata)
            self.vstat = np.array(ydata)


def finish(mash_data, soc_aggregate_off, actual_nom_unit_cap):
    """Reform data into table for application plus the capacity to use in constants"""
    fin_data = []
    for (curve, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in mash_data:
        # The curve that goes to lowest soc is cap since all items start at soc = 1
        soc_scale = (curve.soc - soc_aggregate_off) / (1. - soc_aggregate_off)
        New_raw_data = DataCP(actual_nom_unit_cap, curve.temp_c, soc_scale, curve.vstat, curve.data_file)
        fin_data.append((New_raw_data, Battery.UNIT_CAP_RATED, temp, p_color, p_style, marker, marker_size))

    return fin_data        


def mash(raw_data, v_off_thresh=Battery.VB_OFF):
    """Mash all data together normalized with same ordinates"""

    # Aggregate the normalized soc data
    soc = []
    soc_aggregate_off = 1.  # initial value only
    for (curve, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in raw_data:
        for i in np.arange(len(curve.soc)):
            soc_data = curve.soc[i]
            vstat = curve.vstat[i]
            soc_normal = normalize_soc(soc_data, nom_unit_cap, Battery.UNIT_CAP_RATED)
            if vstat > v_off_thresh-1.:
                soc_aggregate_off = min(soc_aggregate_off, soc_normal)
            soc.append(soc_normal)
    soc_sort = np.unique(np.array(soc))

    # Mash
    mashed_data = []
    for (curve, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in raw_data:
        soc_normal = normalize_soc(curve.soc, nom_unit_cap, Battery.UNIT_CAP_RATED)
        lut_voc_soc = myTables.TableInterp1D(soc_normal, curve.vstat)
        voc_soc_sort = []
        for soc in soc_sort:
            voc_soc_sort.append(lut_voc_soc.interp(soc))
        New_raw_data = DataCP(nom_unit_cap, curve.temp_c, soc_sort, voc_soc_sort, curve.data_file)
        mashed_data.append((New_raw_data, Battery.UNIT_CAP_RATED, temp, p_color, p_style, marker, marker_size))

    return mashed_data, soc_aggregate_off


def normalize_soc(soc_data, nom_unit_cap_data, unit_cap_rated=100.):
    """Normalize for saturation defined to be soc = 1"""
    soc_normal = 1. - (1. - soc_data) * nom_unit_cap_data / unit_cap_rated
    return soc_normal


def plot_all(raw_data, mashed_data, finished_data, act_unit_cap, fig_files=None, plot_title=None, fig_list=None, filename='Calibrate_exe'):
    """Plot the various stages of data reduction"""
    if fig_files is None:
        fig_files = []

    fig_list.append(plt.figure())  # raw data 1
    plt.subplot(111)
    plt.title(plot_title + ' raw')
    for (curve, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in raw_data:
        plt.plot(curve.soc, curve.vstat, color=p_color, linestyle=p_style, marker=marker, markersize=marker_size,
                 label='vstat ' + str(nom_unit_cap) + '  ' + str(temp) + 'C')
    plt.legend(loc=1)
    plt.xlabel('soc data')
    plt.ylabel('vstat unit, v')
    plt.grid()

    fig_list.append(plt.figure())  # normalized data 2
    plt.subplot(111)
    plt.title(plot_title + ' mashed')
    for (curve, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in mashed_data:
        plt.plot(curve.soc, curve.vstat, color=p_color, linestyle=p_style, marker=marker, markersize=marker_size,
                 label='vstat ' + str(nom_unit_cap) + '  ' + str(temp) + 'C')
    plt.legend(loc=1)
    plt.xlabel('soc data normal')
    plt.ylabel('vstat unit, v')
    plt.grid()

    fig_list.append(plt.figure())  # finished data 3
    cap_str = "{:5.1f}".format(act_unit_cap)
    ax = plt.subplot(111)
    # place a text box in upper left in axes coords
    props = dict(boxstyle='round', facecolor='wheat', alpha=0.5)
    ax.text(0.4, 0.2, 'Set NOM_UNIT_CAP = ' + cap_str, transform=ax.transAxes, fontsize=14,
            verticalalignment='top', bbox=props)
    plt.title(plot_title + ' finished')
    for (curve, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in finished_data:
        plt.plot(curve.soc, curve.vstat, color=p_color, linestyle=p_style, marker=marker, markersize=marker_size,
                 label='voc ' + str(temp) + 'C')
    plt.legend(loc=1)
    plt.xlabel('soc')
    plt.ylabel('voc(soc), v')
    plt.grid()

    fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return fig_list, fig_files


def main():

    data_files = [('G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_vstat 11C soc2p2_ch.csv',
                   100., 11., 'blue', '-.', 'o', 4),
                  ('G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_vstat 21p5C soc2p2_ch.csv',
                   100., 21.5, 'orange', '--', 'x', 4),
                  ('G:/My Drive/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_vstat 25C soc2p2_ch.csv',
                   110., 25., 'red', '-', '1', 4),]
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
    hdr_key = 'vstat'  # key to be found in header for data

    # Initialize
    raw_files = []
    Raw_files = []
    fig_list = []
    fig_files = []
    data_file_clean = None
    for (data_file, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in data_files:
        data_file_clean = write_clean_file(data_file, type_='', hdr_key=hdr_key, unit_key=unit_key)
        if data_file_clean is None:
            print(f"case {data_file=} {temp=} is dirty...skipping")
            continue
        else:
            raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
            Raw_data = DataC(nom_unit_cap, temp, raw, data_file_clean)
            raw_files.append((raw, nom_unit_cap, temp))
            Raw_files.append((Raw_data, nom_unit_cap, temp, p_color, p_style, marker, marker_size))

    # Mash all the data to one table and normalize to NOM_UNIT_CAP = 100
    Mashed_data, soc_aggregate_off = mash(Raw_files)
    actual_cap = (1.-soc_aggregate_off) * Battery.UNIT_CAP_RATED
    print("\nAfter mashing:  capacity = {:5.1f}".format(actual_cap))

    # Shift and scale curves for calculated capacity
    Finished_curves = finish(Mashed_data, soc_aggregate_off, actual_cap)

    date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
    data_root = data_file_clean.split('/')[-1].replace('.csv', '_')
    filename = data_root + sys.argv[0].split('/')[-1].split('\\')[-1].split('.')[-2]
    plot_title = filename + '   ' + date_time
    plot_all(Raw_files, Mashed_data, Finished_curves, actual_cap, fig_files=fig_files, plot_title=plot_title, fig_list=fig_list,
             filename='Calibrate_exe')
    plt.show()
    cleanup_fig_files(fig_files)


if __name__ == '__main__':
    main()
