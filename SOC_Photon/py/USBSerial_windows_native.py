import socket

import time
import serial
import io
import time
import os
import numpy as np
from pylive import liven_plotter
import matplotlib.pyplot as plt

s = serial.Serial(port='COM4', baudrate=115200,  bytesize=8, parity='N', stopbits=1, timeout=None, xonxoff=0, rtscts=0)
var_str = "unit,               hm,                  cTime,       dt,       sat,sel,mod,  Tb,  Vb,  Ib,        Vsat,Vdyn,Voc,Voc_ekf,     y_ekf,    soc_m,soc_ekf,soc,soc_wt,"
vars = var_str.replace(" ", "").split(',')

count = 0
i = 0
cTime_last = None
y_v = None
t_v = None
time_span = 300.  # second plot window setting
y_vec1 = None
y_vec2 = None
linen_x1 = None
linen_x2 = None
axx1 = None
axx2 = None
fig = None
identifier = ''
print(var_str)
try:
    while count<5:
        count += 1
        time.sleep(0.8)
        data_r = s.readline().decode().rstrip()
        if data_r.__contains__("pro_"):
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
            Vdyn = float(list_r[11])
            Voc = float(list_r[12])
            Voc_ekf = float(list_r[13])
            y_ekf = float(list_r[14])
            soc_m = float(list_r[15])
            soc_ekf = float(list_r[16])
            soc = float(list_r[17])
            soc_wt = float(list_r[18])
            print(count, unit, hm, cTime, dt, sat, sel, mod, Tb, Vb, Ib, Vsat, Vdyn, Voc, Voc_ekf, y_ekf, soc_m, soc_ekf, soc, soc_wt)
            i += 1
            # Plot when have at least 2 points available
            if i > 1:
                # Setup
                if i == 2:
                    T_maybe = cTime - cTime_last
                    n_v = int(time_span / T_maybe)
                    t_v = np.arange(-n_v*T_maybe, 0, T_maybe)
                    y_vec1 = np.zeros((len(t_v), 1))
                    y_vec1[:, 0] = Ib
                    y_vec2 = np.zeros((len(t_v), 4))
                    y_vec2[:, 0] = Vb
                    y_vec2[:, 1] = Voc
                    y_vec2[:, 2] = Voc_ekf
                    y_vec2[:, 3] = Vsat
                    print('Point#', i, 'at cTime=', cTime, 'T may be=', T_maybe, 'N=', n_v)
                    # print('t_v=', t_v)
                    # print('y_vec1=', y_vec1, 'y_vec2=', y_vec2)
                # Ready for plots
                if linen_x1 is None:
                    fig = plt.figure(figsize=(12, 5))
                linen_x1, axx1 = liven_plotter(t_v, y_vec1, linen_x1, fig, subplot=211, ax=axx1, y_label='Amps',
                                               title='Title: {}'.format(identifier), pause_time=0.1,
                                               labels=['Ib'])
                linen_x2, axx2 = liven_plotter(t_v, y_vec2, linen_x2, fig, subplot=212, ax=axx2, y_label='Volts',
                                               pause_time=0.1, labels=['Vb', 'Voc', 'Voc_ekf', 'Vsat'])
                y_vec1 = np.append(y_vec1[1:][:], np.zeros((1, 2)), axis=0)
                y_vec2 = np.append(y_vec2[1:][:], np.zeros((1, 1)), axis=0)
            # Past values
            cTime_last = cTime

except:
    print('exit')
    exit(1)
