# plot_soc_photon_data:  plot known data structure
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

import time
import numpy as np
from pylive import liven_plotter
import matplotlib.pyplot as plt


def plot_soc_photon_data(r, key):
    var_str = "unit,               hm,                  cTime,       dt,       sat,sel,mod,  Tb,  Vb,  Ib,        Vsat,dV_dyn,Voc_stat,Voc_ekf,     y_ekf,    soc_m,soc_ekf,soc,"
    count = 0
    i = 0
    cTime_last = None
    t_v = None
    T_actual = 0.
    T_actual_past = 0.
    time_span = 3600. * 4.  # second plot window setting
    y_vec0 = None
    y_vec1 = None
    y_vec2 = None
    linen_x0 = None
    linen_x1 = None
    linen_x2 = None
    axx0 = None
    axx1 = None
    axx2 = None
    fig = None
    identifier = ''
    print(var_str)
    try:
        while True:
            count += 1
            time.sleep(0.01)
            try:
                data_r = r.readline().decode().rstrip()  # USB
            except IOError:
                data_r = r.readline().rstrip()  # BLE
            if data_r.__contains__(key):
                list_r = data_r.split(',')
                unit = list_r[0]
                hm = list_r[1]
                cTime = float(list_r[2])
                dt = float(list_r[3])
                sat = int(list_r[4])
                sel = int(list_r[5])
                mod = int(list_r[6])
                Tb = float(list_r[7])
                Vb = float(list_r[8])
                Ib = float(list_r[9])
                Vsat = float(list_r[10])
                dV_dyn = float(list_r[11])
                Voc_stat = float(list_r[12])
                Voc_ekf = float(list_r[13])
                y_ekf = float(list_r[14])
                soc_m = float(list_r[15])
                soc_ekf = float(list_r[16])
                soc = float(list_r[17])
                print(count, unit, hm, cTime, dt, sat, sel, mod, Tb, Vb, Ib, Vsat, dV_dyn, Voc_stat, Voc_ekf, y_ekf, soc_m,
                      soc_ekf, soc)
                i += 1
                # Plot when have at least 2 points available
                if i > 1:
                    # Setup
                    if i == 2:
                        T_maybe = cTime - cTime_last
                        T_actual_past = T_maybe
                        n_v = int(time_span / T_maybe)
                        t_v = np.arange(-n_v * T_maybe, 0, T_maybe)
                        y_vec0 = np.zeros((len(t_v), 3))
                        y_vec0[:, 0] = soc_m
                        y_vec0[:, 1] = soc_ekf
                        y_vec0[:, 2] = soc
                        y_vec1 = np.zeros((len(t_v), 1))
                        y_vec1[:, 0] = Ib
                        y_vec2 = np.zeros((len(t_v), 4))
                        y_vec2[:, 0] = Vb
                        y_vec2[:, 1] = Voc_stat
                        y_vec2[:, 2] = Voc_ekf
                        y_vec2[:, 3] = Vsat
                        print('Point#', i, 'at cTime=', cTime, 'T may be=', T_maybe, 'N=', n_v)
                        # print('t_v=', t_v)
                        # print('y_vec1=', y_vec1, 'y_vec2=', y_vec2)
                    # Ready for plots
                    T_actual = cTime - cTime_last
                    dt = T_actual_past - T_actual
                    t_v[:] = t_v[:] + dt
                    y_vec0[-1][0] = soc_m
                    y_vec0[-1][1] = soc_ekf
                    y_vec0[-1][2] = soc
                    y_vec1[-1][0] = Ib
                    y_vec2[-1][0] = Vb
                    y_vec2[-1][1] = Voc_stat
                    y_vec2[-1][2] = Voc_ekf
                    y_vec2[-1][3] = Vsat
                    if linen_x1 is None:
                        fig = plt.figure(figsize=(12, 5))
                    linen_x0, axx0 = liven_plotter(t_v, y_vec0, linen_x0, fig, subplot=311, ax=axx0, y_label='Amps',
                                                   title='Title: {}'.format(identifier), pause_time=0.01,
                                                   labels=['soc_m', 'soc_ekf', 'soc'])
                    linen_x1, axx1 = liven_plotter(t_v, y_vec1, linen_x1, fig, subplot=312, ax=axx1, y_label='Amps',
                                                   pause_time=0.01,
                                                   labels='Ib')
                    linen_x2, axx2 = liven_plotter(t_v, y_vec2, linen_x2, fig, subplot=313, ax=axx2, y_label='Volts',
                                                   pause_time=0.01, labels=['Vb', 'Voc_stat', 'Voc_ekf', 'Vsat'])
                    y_vec0 = np.append(y_vec0[1:][:], np.zeros((1, 3)), axis=0)
                    y_vec1 = np.append(y_vec1[1:][:], np.zeros((1, 1)), axis=0)
                    y_vec2 = np.append(y_vec2[1:][:], np.zeros((1, 4)), axis=0)
                # Past values
                cTime_last = cTime
                T_actual_past = T_actual
    except Exception as err:
        print("Something went wrong: ", err)
        r.close()
        exit(1)
