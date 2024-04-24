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
from Colors import *
import numpy as np
import sys
import os


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


def finish(mash_data, soc_rated_off, actual_nom_unit_cap):
    """Reform data into table for application plus the capacity to use in constants"""
    fin_data = []
    for (curve, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in mash_data:
        # The curve that goes to lowest soc is cap since all items start at soc = 1
        soc_scale = (curve.soc - soc_rated_off) / (1. - soc_rated_off)
        New_raw_data = DataCP(actual_nom_unit_cap, curve.temp_c, soc_scale, curve.vstat, curve.data_file)
        fin_data.append((New_raw_data, Battery.UNIT_CAP_RATED, temp, p_color, p_style, marker, marker_size))

    return fin_data        


def mash(raw_data, v_off_thresh=Battery.VB_OFF, rated_temp=25.):
    """Mash all data together normalized with same ordinates"""

    # Aggregate the normalized soc data
    soc = []
    soc_rated_off = 1.  # initial value only
    for (curve, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in raw_data:
        for i in np.arange(len(curve.soc)):
            soc_data = curve.soc[i]
            vstat = curve.vstat[i]
            soc_normal = normalize_soc(soc_data, nom_unit_cap, Battery.UNIT_CAP_RATED)
            if vstat > v_off_thresh-1. and abs(temp - rated_temp) < 3:
                soc_rated_off = min(soc_rated_off, soc_normal)
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

    return mashed_data, soc_rated_off


def minimums(fin_data):
    """Find minimum soc schedule"""
    t_min = []
    soc_min = []
    for (curve, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in fin_data:
        t_min.append(temp)
        lut_voc_soc = myTables.TableInterp1D(curve.vstat, curve.soc)
        soc_min.append(lut_voc_soc.interp(Battery.VB_OFF))

    return t_min, soc_min


def normalize_soc(soc_data, nom_unit_cap_data, unit_cap_rated=100.):
    """Normalize for saturation defined to be soc = 1"""
    soc_normal = 1. - (1. - soc_data) * nom_unit_cap_data / unit_cap_rated
    return soc_normal


def plot_all(raw_data, mashed_data, finished_data, red_data, act_unit_cap, fig_files=None, plot_title=None,
             fig_list=None, filename='Calibrate_exe'):
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

    fig_list.append(plt.figure())  # finished data 3
    cap_str = "{:5.1f}".format(act_unit_cap)
    ax = plt.subplot(111)
    # place a text box in upper left in axes coords
    props = dict(boxstyle='round', facecolor='wheat', alpha=0.5)
    ax.text(0.4, 0.2, 'Set NOM_UNIT_CAP = ' + cap_str, transform=ax.transAxes, fontsize=14,
            verticalalignment='top', bbox=props)
    plt.title(plot_title + ' reduced')
    for (curve, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in red_data:
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


def reduce(fin_data, breakpoints):
    """Sample finished curves to minimize breakpoints"""
    red_data = []
    for (curve, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in fin_data:
        val = []
        lut_voc_soc = myTables.TableInterp1D(curve.soc, curve.vstat)
        for brk in breakpoints:
            val.append(lut_voc_soc.interp(brk))
        New_red_data = DataCP(nom_unit_cap, curve.temp_c, breakpoints, val, curve.data_file)
        red_data.append((New_red_data, Battery.UNIT_CAP_RATED, temp, p_color, p_style, marker, marker_size))
    return red_data


def main(data_files=None):
    #  data_files[( data_file, nom_unit_cap, temp_c, p_color, marker, marker_size), (...), ...]  List of tuples of data from experiments
    #   data_file       Full path to data file
    #   nom_unit_cap    Nominal applied battery unit (100 Ah unit) capacity, Ah
    #   temp_c          Temperature bulk, Tb, in deg C
    #   p_color         matplotlib line color
    #   marker          matplotlib marker
    #   marker_size     matplotlib marker size

    # Hand-select these from figure 3 and re-run this script
    breakpoints = [-.1136, -.044, 0, .0164, .032, .055, .064, .114, .134, .1545, .183, .2145, .3, .4, .5, .6, .7, .8, .9, .96, .98, 1]

    hdr_key = 'vstat'  # key to be found in header for data

    # Sort by temperature lowest to highest for consistency throughout
    data_files = sorted(data_files, key=lambda x: x[2])

    # Initialize
    raw_files = []
    Raw_files = []
    fig_list = []
    fig_files = []
    data_root = None
    data_file_clean = None
    CHM = None
    for (data_file, nom_unit_cap, temp, p_color, p_style, marker, marker_size) in data_files:
        (data_file_folder, data_file_txt) = os.path.split(data_file)
        unit_key = data_file_txt.split(' ')[-1].split('.')[0]
        unit = unit_key.split('_')[0]
        CHM = unit_key.split('_')[1].upper()
        data_file_clean = write_clean_file(data_file, type_='', hdr_key=hdr_key, unit_key=unit_key)
        if data_file_clean is None:
            print(f"case {data_file=} {temp=} is dirty...skipping")
            continue
        else:
            data_root = data_file_clean.split('/')[-1].replace('.csv', '_').split(' ')[-1]
            raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
            Raw_data = DataC(nom_unit_cap, temp, raw, data_file_clean)
            raw_files.append((raw, nom_unit_cap, temp))
            Raw_files.append((Raw_data, nom_unit_cap, temp, p_color, p_style, marker, marker_size))

    # Mash all the data to one table and normalize to NOM_UNIT_CAP = 100
    Mashed_data, soc_rated_off = mash(Raw_files)
    actual_cap = (1.-soc_rated_off) * Battery.UNIT_CAP_RATED

    # Shift and scale curves for calculated capacity
    Finished_curves = finish(Mashed_data, soc_rated_off, actual_cap)

    # Minimize number of points
    Massaged_curves = reduce(Finished_curves, breakpoints)

    # Find minimums
    t_min, soc_min = minimums(Finished_curves)

    # Print
    n_n = len(t_min)
    m_t = n_n
    soc_brk = Massaged_curves[0][0].soc
    n_s = len(soc_brk)
    date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
    print("\nfor Chemistry_BMS.cpp:\n")
    print(Colors.fg.green, "// {:s}:  tune to data".format(date_time))
    t_min_str = ''.join(f'{q:.1f}, ' for q in t_min)
    soc_min_str = ''.join(f'{q:.2f}, ' for q in soc_min)
    soc_brk_str = ''.join(f'{q:6.3f}, ' for q in soc_brk)
    voc_soc_brk_str = []
    for i in np.arange(m_t):
        voc_soc_brk = Massaged_curves[i][0].vstat
        voc_soc_brk_str.append(''.join(f'{q:6.3f}, ' for q in voc_soc_brk))
    print("const uint8_t M_T_{:s} = {:d};    // Number temperature breakpoints for voc table".format(CHM, m_t))
    print("const uint8_t N_S_{:s} = {:d};   // Number soc breakpoints for voc table".format(CHM, n_s))
    print("const float Y_T_{:s}[M_T_{:s}] = // Temperature breakpoints for voc table\n    {{{:s}}};".format(CHM, CHM, t_min_str))
    print("const float X_SOC_{:s}[N_S_{:s}] = // soc breakpoints for voc table\n    {{{:s}}}; ".format(CHM, CHM, soc_brk_str))
    print("const float T_VOC_{:s}[M_T_{:s} * N_S_{:s}] = // soc breakpoints for soc_min table\n    {{".format(CHM, CHM, CHM))
    for i in np.arange(m_t):
        print("     {:s}".format(voc_soc_brk_str[i]))
    print("    };")
    print("const uint8_t N_N_{:s} = {:d}; // Number of temperature breakpoints for x_soc_min table".format(CHM, n_n))
    print("const float X_SOC_MIN_CH[N_N_{:s}] = {{{:s}}};  // Temperature breakpoints for soc_min table".format(CHM, t_min_str))
    print("const float T_SOC_MIN_CH[N_N_{:s}] = {{{:s}}};  // soc_min(t)".format(CHM, soc_min_str))
    print(Colors.reset, "\n\nfor local_config.h.{:s}\n".format(unit))
    print(Colors.fg.green, "#define NOM_UNIT_CAP          {:5.1f}   // Nominal battery unit capacity at RATED_TEMP.  (* 'Sc' or '*BS'/'*BP'), Ah\n".format(actual_cap))
    print(Colors.reset)

    filename = data_root + sys.argv[0].split('/')[-1].split('\\')[-1].split('.')[-2]
    plot_title = filename + '   ' + date_time
    plot_all(Raw_files, Mashed_data, Finished_curves, Massaged_curves, actual_cap, fig_files=fig_files,
             plot_title=plot_title, fig_list=fig_list,
             filename='Calibrate_exe')
    plt.show()
    cleanup_fig_files(fig_files)


if __name__ == '__main__':
    if sys.platform == 'linux':
        gdrive = '/home/daveg/google-drive'
    else:
        gdrive = 'G:/My Drive'

    data_files = [
        (gdrive + '/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_vstat 21p5C soc2p2_ch.csv',
            100., 21.5, 'blue', '-.', 'x', 4),
        (gdrive + '/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_vstat 25C pro3p2_ch.csv',
            100., 25., 'green', '-', 's', 4),
        # (gdrive + '/GitHubArchive/SOC_Particle/dataReduction/g20240331/soc_vstat 35C pro3p2_ch.csv',
        #  101.6, 35., 'red', '--', 'o', 4),
    ]

    main(data_files=data_files)
