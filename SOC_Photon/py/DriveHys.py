# Drive Hysteresis class to match data
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

__author__ = 'Dave Gutz <davegutz@alum.mit.edu>'
__version__ = '$Revision: 1.1 $'
__date__ = '$Date: 2022/05/30 13:15:02 $'

import numpy as np
from pyDAGx.lookup_table import LookupTable
from unite_pictures import cleanup_fig_files


class Hysteresis:
    # Use variable resistor to create hysteresis from an RC circuit

    # def __init__(self, t_dv=None, t_soc=None, t_r=None, cap=3.6e5, scale=1., dv_hys=0.0):  # old nominal before 6/26
    def __init__(self, t_dv=None, t_soc=None, t_r=None, cap=3.6e5, scale=1., dv_hys=-0.05):
        # Defaults
        if t_dv is None:
            t_dv = [-0.9, -0.7,     -0.5,   -0.3,   0.0,    0.3,    0.5,    0.7,    0.9]
        if t_soc is None:
            t_soc = [0, .5, 1]
        if t_r is None:
            t_r = [1e-6, 0.064,    0.050,  0.036,  0.015,  0.024,  0.030,  0.046,  1e-6,
                   1e-6, 1e-6,     0.050,  0.036,  0.015,  0.024,  0.030,  1e-6,   1e-6,
                   1e-6, 1e-6,     1e-6,   0.036,  0.015,  0.024,  1e-6,   1e-6,   1e-6]
        self.scale = scale
        for i in range(len(t_dv)):
            t_dv[i] *= self.scale
            t_r[i] *= self.scale
        self.disabled = self.scale < 1e-5
        self.lut = LookupTable()
        self.lut.addAxis('x', t_dv)
        self.lut.addAxis('y', t_soc)
        self.lut.setValueTable(t_r)
        if self.disabled:
            self.cap = cap
        else:
            self.cap = cap / self.scale  # maintain time constant = R*C
        self.res = 0.
        self.soc = 0.
        self.ib = 0.
        self.ioc = 0.
        self.dv_hys = dv_hys
        self.dv_dot = 0.
        self.saved = Saved()
        self.voc = 0.
        self.voc_stat_est = 0.
        self.voc_stat_target = 0.

    def __str__(self, prefix=''):
        s = prefix + "Hysteresis:\n"
        res = self.look_hys(dv=0., soc=0.8)
        s += "  res(median) =  {:6.4f}  // Null resistance, Ohms\n".format(res)
        s += "  cap      = {:10.1f}  // Capacitance, Farads\n".format(self.cap)
        s += "  tau      = {:10.1f}  // Null time constant, sec\n".format(res*self.cap)
        s += "  ib       =    {:7.3f}  // Current in, A\n".format(self.ib)
        s += "  ioc      =    {:7.3f}  // Current out, A\n".format(self.ioc)
        s += "  soc      =   {:8.4f}  // State of charge input, dimensionless\n".format(self.soc)
        s += "  res      =    {:7.3f}  // Variable resistance value, ohms\n".format(self.res)
        s += "  dv_dot   =    {:7.3f}  // Calculated voltage rate, V/s\n".format(self.dv_dot)
        s += "  dv_hys   =    {:7.3f}  // Delta voltage state, V\n".format(self.dv_hys)
        s += "  disabled =     {:2.0f}      // Hysteresis disabled by low scale input < 1e-5, T=disabled\n".format(self.disabled)
        s += "  hys_scale=    {:7.3f}  // Scalar on hys\n".format(self.scale)
        return s

    def calculate_hys(self, ib, soc):
        self.ib = ib
        self.soc = soc
        if self.disabled:
            self.res = 0.
            self.ioc = ib
            self.dv_dot = 0.
        else:
            self.res = self.look_hys(self.dv_hys, self.soc)
            self.ioc = self.dv_hys / self.res
            self.dv_dot = (self.ib - self.ioc) / self.cap
        return self.dv_dot

    def init(self, dv_init):
        self.dv_hys = dv_init

    def look_hys(self, dv, soc):
        if self.disabled:
            self.res = 0.
        else:
            self.res = self.lut.lookup(x=dv/self.scale, y=soc)*self.scale
        return self.res

    def save(self, time):
        self.saved.time.append(time)
        self.saved.soc.append(self.soc)
        self.saved.res.append(self.res)
        self.saved.dv_hys.append(self.dv_hys)
        self.saved.dv_dot.append(self.dv_dot)
        self.saved.ib.append(self.ib)
        self.saved.ioc.append(self.ioc)
        self.saved.voc.append(self.voc)
        self.saved.voc_stat_est.append(self.voc_stat_est)
        self.saved.voc_stat_target.append(self.voc_stat_target)

    def update(self, dt, voc, voc_stat_target):
        self.dv_hys += self.dv_dot * dt
        self.voc = voc
        self.voc_stat_est = self.voc - self.dv_hys
        self.voc_stat_target = voc_stat_target
        return self.dv_hys


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.dv_hys = []
        self.dv_dot = []
        self.res = []
        self.soc = []
        self.ib = []
        self.ioc = []
        self.voc = []
        self.voc_stat_est = []
        self.voc_stat_target = []


if __name__ == '__main__':
    import sys
    import doctest
    from datetime import datetime
    from unite_pictures import unite_pictures_into_pdf

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt

    def overall(hys=Hysteresis().saved, filename='', fig_files=None, plot_title=None, n_fig=None):
        if fig_files is None:
            fig_files = []

        plt.figure()
        n_fig += 1
        plt.subplot(221)
        plt.title(plot_title)
        plt.plot(hys.time, hys.soc, color='red', label='soc')
        plt.legend(loc=3)
        plt.subplot(222)
        plt.plot(hys.time, hys.res, color='black', label='res, Ohm')
        plt.legend(loc=3)
        plt.subplot(223)
        plt.plot(hys.time, hys.ib, color='blue', label='ib, A')
        plt.plot(hys.time, hys.ioc, color='green', label='ioc, A')
        plt.legend(loc=2)
        plt.subplot(224)
        plt.plot(hys.time, hys.dv_hys, color='red', label='dv_hys, V')
        plt.legend(loc=2)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()
        n_fig += 1
        plt.subplot(111)
        plt.title(plot_title)
        plt.plot(hys.soc, hys.dv_hys, color='red', label='dv_hys vs soc')
        plt.legend(loc=2)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()
        n_fig += 1
        plt.subplot(111)
        plt.title(plot_title)
        plt.plot(hys.time, hys.voc, color='red', label='voc')
        plt.plot(hys.time, hys.voc_stat_est, color='blue', label='voc_stat_est')
        plt.plot(hys.time, hys.voc_stat_target, color='green', linestyle='--', label='voc_stat_target')
        plt.legend(loc=2)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        return n_fig, fig_files

    def main():
        # Transient  inputs
        data_file_clean = '../dataReduction/real world Xp20 20220626_sim.txt'
        cols_sim = ('unit_m', 'c_time', 'Tb_s', 'voc_stat_s', 'dv_dyn_s', 'vb_s', 'ib_s', 'soc_s', 'reset_s')
        mon_old = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols_sim, dtype=None,
                                encoding=None).view(np.recarray)
        t_v = mon_old.c_time - mon_old.c_time[0]
        vb_t = mon_old.vb_m   # not useful except to back out voc
        ib_t = mon_old.ib_s
        voc_stat_target_t = mon_old.voc_stat_m
        soc_t = mon_old.soc_s
        dv_dyn_t = mon_old.dv_dyn_m
        voc_t = vb_t - dv_dyn_t
        hys = Hysteresis(dv_hys=-0.0, cap=3.6e5)
        dt = t_v[1] - t_v[0]

        # time loop
        for i in range(len(t_v)):
            t = t_v[i]
            if i > 0:
                dt = t - t_v[i-1]
            ib = ib_t[i]
            voc_stat_target = voc_stat_target_t[i]
            soc = soc_t[i]
            voc = voc_t[i]

            # Models
            hys.calculate_hys(ib=ib, soc=soc)
            hys.update(dt=dt, voc=voc, voc_stat_target=voc_stat_target)

            # Plot stuff
            hys.save(t)

        # Data
        print('hys:  ', str(hys))

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time

        n_fig, fig_files = overall(hys.saved, filename, fig_files, plot_title=plot_title, n_fig=n_fig)

        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='figures')
        cleanup_fig_files(fig_files)
        plt.show()

    main()
