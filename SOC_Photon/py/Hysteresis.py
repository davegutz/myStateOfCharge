# Hysteresis class to model battery charging / discharge hysteresis
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
__date__ = '$Date: 2022/11/12 13:15:02 $'

import numpy as np
# from pyDAGx.lookup_table import LookupTable
from pyDAGx.myTables import TableInterp2D, TableInterp1D
from unite_pictures import cleanup_fig_files

HYS_DV_MIN = 0.2


class Hysteresis:
    # Use variable resistor to create hysteresis from an RC circuit

    def __init__(self, scale=1., dv_hys=0.0, chem=0, scale_cap=1.,
                 t_dv0=None, t_soc0=None, t_r0=None, t_dv_min0=None, t_dv_max0=None,
                 t_dv1=None, t_soc1=None, t_r1=None, t_dv_min1=None, t_dv_max1=None):
        # Defaults
        self.chm = chem
        self.scale_cap = scale_cap

        # Battleborn
        self.cap0 = 3.6e3
        if t_dv0 is None:
            t_dv0 = [-0.7,   -0.5,  -0.3,  0.0,   0.15,   0.3,   0.7]
        if t_soc0 is None:
            t_soc0 = [0, .5, 1]
        if t_r0 is None:
            t_r0 = [0.019, 0.015, 0.016, 0.009, 0.011, 0.017, 0.030,
                    0.014, 0.014, 0.010, 0.008, 0.010, 0.015, 0.015,
                    0.016, 0.016, 0.016, 0.005, 0.010, 0.010, 0.010]
        if t_dv_min0 is None:
            t_dv_min0 = [-0.7, -0.5, -0.3]
        if t_dv_max0 is None:
            t_dv_max0 = [0.7, 0.3, 0.15]
        self.lut0 = TableInterp2D(t_dv0, t_soc0, t_r0)
        self.lu_x0 = TableInterp1D(t_soc0, t_dv_max0)
        self.lu_n0 = TableInterp1D(t_soc0, t_dv_min0)

        # CHINS
        self.cap1 = 3.6e3
        if t_dv1 is None:
            # t_dv1 = [-0.7,   -0.5,  -0.3,  0.0,   0.3,   0.6,   1.7]
            t_dv1 = [-0.7,   -0.5,  -0.3,  0.0,   0.15,   0.3,   0.85]
        if t_soc1 is None:
            t_soc1 = [0, .5, 1]
            # tune tip:  use center point to set time constant.  rest of points for magnitude

            # t_dv1 = [-0.7,   -0.5,  -0.3,  0.0,   0.3,   0.6,   1.7]
            sch = [0.018, 0.018, 0.018, 0.010, 0.024, 0.028, 0.028]  # figure
            sch = [0.016, 0.016, 0.016, 0.008, 0.020, 0.025, 0.025]  # too negative
            sch = [0.019, 0.019, 0.019, 0.008, 0.016, 0.020, 0.020]  # too negative, too positive
            sch = [0.018, 0.018, 0.018, 0.008, 0.018, 0.022, 0.022]  # too negative, not enough positive
            sch = [0.014, 0.014, 0.014, 0.008, 0.020, 0.025, 0.025]  # too negative, not enough positive
            sch = [0.012, 0.012, 0.012, 0.008, 0.022, 0.027, 0.027]  # ok negative, not enough positive
            sch = [0.012, 0.012, 0.012, 0.008, 0.024, 0.029, 0.029]  # ok negative, ok positive

            # t_dv1 = [-0.7,   -0.5,  -0.3,  0.0,   0.15,   0.3,   0.85]
            # sch = [0.012, 0.012, 0.012, 0.008, 0.020, 0.024, 0.029]  # ok negative, ? positive
            # sch = [0.012, 0.012, 0.012, 0.008, 0.012, 0.024, 0.029]  # ok negative,  not enough positive
            # sch = [0.012, 0.012, 0.012, 0.008, 0.016, 0.024, 0.029]  # ok negative,  almost enough positive 1 too much positive 2
            sch = [0.012, 0.012, 0.012, 0.008, 0.014, 0.024, 0.029]  # ok negative,  not enough positive 1 good positive 2

        if t_r1 is None:
            t_r1 = sch+sch+sch
        if t_dv_min1 is None:
            t_dv_min1 = [-0.7, -0.5, -0.3]
        if t_dv_max1 is None:
            t_dv_max1 = [1.7, 0.6, 0.3]
        self.lut1 = TableInterp2D(t_dv1, t_soc1, t_r1)
        self.lu_x1 = TableInterp1D(t_soc1, t_dv_max1)
        self.lu_n1 = TableInterp1D(t_soc1, t_dv_min1)

        if self.chm == 0:
            self.cap = self.cap0
            self.lut = self.lut0
            self.lu_x = self.lu_x0
            self.lu_n = self.lu_n0
        elif self.chm == 1:
            self.cap = self.cap1
            self.lut = self.lut1
            self.lu_x = self.lu_x1
            self.lu_n = self.lu_n1

        self.scale = scale
        self.disabled = self.scale < 1e-5
        self.res = 0.
        self.soc = 0.
        self.ib = 0.
        self.ioc = 0.
        self.dv_hys = dv_hys
        self.dv_dot = 0.
        self.saved = Saved()

    def __str__(self, prefix=''):
        s = prefix + "Hysteresis:\n"
        res = self.look_hys(dv=0., soc=0.8, chem=self.chm)
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

    def calculate_hys(self, ib, soc, chem=0):
        self.chm = chem
        self.ib = ib
        self.soc = soc
        if self.disabled:
            self.res = 0.
            self.ioc = ib
            self.dv_dot = 0.
        else:
            self.res = self.look_hys(self.dv_hys, self.soc, self.chm)
            self.ioc = self.dv_hys / self.res
            self.dv_dot = (self.ib - self.dv_hys/self.res) / self.cap
        return self.dv_dot

    def init(self, dv_init):
        self.dv_hys = dv_init

    def look_hys(self, dv, soc, chem=0):
        self.chm = chem
        if self.disabled:
            self.res = 0.
        else:
            if chem == 0:
                self.cap = self.cap0
                self.res = self.lut0.interp(x=dv, y=soc)
            elif chem == 1:
                self.cap = self.cap1
                self.res = self.lut1.interp(x=dv, y=soc)
        return self.res

    def save(self, time):
        self.saved.time.append(time)
        self.saved.soc.append(self.soc)
        self.saved.res.append(self.res)
        self.saved.dv_hys.append(self.dv_hys)
        self.saved.dv_dot.append(self.dv_dot)
        self.saved.ib.append(self.ib)
        self.saved.ioc.append(self.ioc)

    def update(self, dt, trusting_sensors=False, init_high=False, init_low=False, e_wrap=0., chem=0):
        self.chm = chem
        if self.chm == 0:
            self.cap = self.cap0
            dv_max = self.lu_x0.interp(x=self.soc)
            dv_min = self.lu_n0.interp(x=self.soc)
        elif self.chm == 1:
            self.cap = self.cap1
            dv_max = self.lu_x1.interp(x=self.soc)
            dv_min = self.lu_n1.interp(x=self.soc)

        # Reset if at endpoints.   e_wrap is an actual measurement of hysteresis if trust sensors.  But once
        # dv_hys is reset it regenerates e_wrap so e_wrap in logic breaks that.   Also, dv_hys regenerates dv_dot
        # so break that too by setting it to 0
        if init_low:
            self.dv_hys = max(HYS_DV_MIN, -e_wrap)
            self.dv_dot = 0.  # break positive feedback loop
        if init_high:
            self.dv_hys = -HYS_DV_MIN
            self.dv_dot = 0.  # break positive feedback loop

        # normal ODE integration
        self.dv_hys += self.dv_dot * dt
        self.dv_hys = max(min(self.dv_hys, dv_max), dv_min)

        return self.dv_hys*self.scale


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

        return n_fig, fig_files


    class Pulsar:
        def __init__(self):
            self.time_last_hold = 0.
            self.time_last_rest = -100000.
            self.holding = False
            self.resting = True
            self.index = -1
            self.amp = [100., 0., -100., -100., -100., -100., -100., -100., -100., -100., -100., -100.,
                        100., 100., 100., 100., 100., 100., 100., 100., 100., 100.]
            self.dur = [16000., 0., 600., 600., 600., 600., 600., 600., 600., 600., 600., 600.,
                        600., 600., 600., 600., 600., 600., 600., 600., 600., 600.]
            self.rst = [600., 7200., 3600., 3600., 3600., 3600., 3600., 3600., 3600., 3600., 3600., 7200.,
                        3600., 3600., 3600., 3600., 3600., 3600., 3600., 3600., 3600., 46800.]
            self.pulse_value = self.amp[0]
            self.end_time = self.time_end()

        def calculate(self, time):
            if self.resting and time >= self.time_last_rest + self.rst[self.index]:
                if time < self.end_time:
                    self.index += 1
                self.resting = False
                self.holding = True
                self.time_last_hold = time
                self.pulse_value = self.amp[self.index]
            elif self.holding and time >= self.time_last_hold + self.dur[self.index]:
                self.index += 0  # only advance after resting
                self.resting = True
                self.holding = False
                self.time_last_rest = time
                self.pulse_value = 0.
            return self.pulse_value

        def time_end(self):
            time = 0
            for du in self.dur:
                time += du
            for rs in self.rst:
                time += rs
            return time


    def main():
        # Setup to run the transients
        dt = 10
        # time_end = 2
        # time_end = 500000
        pull = Pulsar()
        time_end = pull.time_end()

        hys = Hysteresis()

        # Executive tasks
        t = np.arange(0, time_end + dt, dt)
        soc = 0.2
        current_in_s = []

        # time loop
        for i in range(len(t)):
            current_in = pull.calculate(t[i])

            init_ekf = (t[i] <= 1)
            if init_ekf:
                hys.init(0.0)

            # Models
            soc = min(max(soc + current_in / 100. * dt / 20000., 0.), 1.)
            hys.calculate_hys(ib=current_in, soc=soc)
            hys.update(dt=dt)

            # Plot stuff
            current_in_s.append(current_in)
            hys.save(t[i])

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
