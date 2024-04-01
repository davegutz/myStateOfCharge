# Hysteresis class to model battery charging / discharge hysteresis
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

__author__ = 'Dave Gutz <davegutz@alum.mit.edu>'
__version__ = '$Revision: 1.1 $'
__date__ = '$Date: 2022/11/12 13:15:02 $'

import numpy as np
from unite_pictures import cleanup_fig_files


class Hysteresis:
    # Use variable resistor to create hysteresis from an RC circuit

    def __init__(self, scale=1., dv_hys=0.0, chem=0, scale_cap=1., s_cap_chg=1., s_cap_dis=1., s_hys_chg=1., s_hys_dis=1.,
                 chemistry=None):
        # Defaults
        self.chm = chem
        self.scale_cap = scale_cap
        self.s_cap_chg = s_cap_chg
        self.s_cap_dis = s_cap_dis
        self.s_hys_chg = s_hys_chg
        self.s_hys_dis = s_hys_dis
        self.cap = chemistry.cap
        self.lut = chemistry.lut_r_hys
        self.luts = chemistry.lut_s_hys
        self.lu_x = chemistry.lu_x_hys
        self.lu_n = chemistry.lu_n_hys
        self.dv_min_abs = chemistry.dv_min_abs
        self.scale = scale
        self.disabled = self.scale < 1e-5
        self.res = 0.
        self.slr = 1.
        self.soc = 0.
        self.ib = 0.
        self.ibs = 0.
        self.ioc = 0.
        self.dv_hys = dv_hys
        self.dv_dot = 0.
        self.tau = 0.
        self.saved = Saved()

    def __str__(self, prefix=''):
        s = prefix + "Hysteresis:\n"
        res, slr = self.look_hys(dv=0., soc=0.8, chem=self.chm)
        s += "  res(median) =  {:6.4f}  // Null resistance, Ohms\n".format(res)
        s += "  chm      =    {:7.3f}  // Chemistry\n".format(self.chm)
        s += "  cap      = {:10.1f}  // Capacitance, Farads\n".format(self.cap)
        s += "  tau      = {:10.1f}  // Null time constant, sec\n".format(res*self.cap)
        s += "  ib       =    {:7.3f}  // Current in, A\n".format(self.ib)
        s += "  ibs      =    {:7.3f}  // Scaled current in, A\n".format(self.ibs)
        s += "  ioc      =    {:7.3f}  // Current out, A\n".format(self.ioc)
        s += "  slr      =    {:7.3f}  // Variable input current scalar\n".format(self.slr)
        s += "  soc      =   {:8.4f}  // State of charge input, dimensionless\n".format(self.soc)
        s += "  res      =    {:7.3f}  // Variable resistance value, ohms\n".format(self.res)
        s += "  dv_dot   =    {:7.3f}  // Calculated voltage rate, V/s\n".format(self.dv_dot)
        s += "  dv_hys   =    {:7.3f}  // Delta voltage state, V\n".format(self.dv_hys)
        s += "  disabled =     {:2.0f}      // Hysteresis disabled by low scale input < 1e-5, T=disabled\n".format(self.disabled)
        s += "  hys_scale=    {:7.3f}  // Scalar on hys\n".format(self.scale)
        s += "  scale_cap=    {:7.3f}  // Scalar on cap\n".format(self.scale_cap)
        return s

    def calculate_hys(self, ib, soc, chem=0, nS=1):
        self.chm = chem
        self.ib = ib
        self.soc = soc
        if self.disabled:
            self.res = 0.
            self.slr = 1.
            self.ibs = self.ib / nS
            self.ioc = ib / nS
            self.dv_dot = 0.
        else:
            self.res, self.slr = self.look_hys(self.dv_hys, self.soc, self.chm)
            self.ioc = self.dv_hys / self.res
            self.ibs = self.ib * self.slr / nS
            self.dv_dot = (self.ibs - self.ioc) / (self.cap*self.scale_cap)
            self.tau = self.res * self.cap * self.scale_cap
            if self.dv_hys >= 0.:
                self.dv_dot /= self.s_cap_chg
            else:
                self.dv_dot /= self.s_cap_dis

        return self.dv_dot

    def init(self, dv_init):
        self.dv_hys = dv_init

    def look_hys(self, dv, soc, chem=0):
        self.chm = chem
        if self.disabled:
            self.res = 0.
            self.slr = 1.
        else:
            self.res = self.lut.interp(x_=dv, y_=soc)
            self.slr = self.luts.interp(x_=dv, y_=soc)
        return self.res, self.slr

    def save(self, time):
        self.saved.time.append(time)
        self.saved.soc.append(self.soc)
        self.saved.res.append(self.res)
        self.saved.slr.append(self.slr)
        self.saved.dv_hys.append(self.dv_hys)
        self.saved.dv_dot.append(self.dv_dot)
        self.saved.ib.append(self.ib)
        self.saved.ibs.append(self.ibs)
        self.saved.ioc.append(self.ioc)
        self.saved.tau.append(self.tau)

    def update(self, dt, init_high=False, init_low=False, e_wrap=0., chem=0):
        self.chm = chem
        dv_max = self.lu_x.interp(x_=self.soc)
        dv_min = self.lu_n.interp(x_=self.soc)

        # Aliasing - return
        if self.tau < dt * 4.:
            if self.ib >= 0:
                self.dv_hys = dv_max
            else:
                self.dv_hys = dv_min
            return self.dv_hys, self.tau

        # Reset if at endpoints.   e_wrap is an actual measurement of hysteresis if trust sensors.  But once
        # dv_hys is reset it regenerates e_wrap so e_wrap in logic breaks that.   Also, dv_hys regenerates dv_dot
        # so break that too by setting it to 0

        if init_low:
            self.dv_hys = max(self.dv_min_abs, -e_wrap)
            self.dv_dot = 0.  # break positive feedback loop
        if init_high:
            self.dv_hys = -self.dv_min_abs
            self.dv_dot = 0.  # break positive feedback loop

        # normal ODE integration
        self.dv_hys += self.dv_dot * dt
        self.dv_hys = max(min(self.dv_hys, dv_max), dv_min)
        if self.dv_hys >= 0.:
            return max(min(self.dv_hys*self.scale*self.s_hys_chg, dv_max), dv_min), self.tau
        else:
            return max(min(self.dv_hys*self.scale*self.s_hys_dis, dv_max), dv_min), self.tau


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.dv_hys = []
        self.dv_dot = []
        self.res = []
        self.slr = []
        self.soc = []
        self.ib = []
        self.ibs = []
        self.ioc = []
        self.tau = []


if __name__ == '__main__':
    import sys
    import doctest
    from datetime import datetime
    from unite_pictures import unite_pictures_into_pdf

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt


    def overall(hys=Hysteresis().saved, filename='', fig_files=None, plot_title=None, fig_list=None):
        if fig_files is None:
            fig_files = []

        fig_list.append(plt.figure())
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
        fig_file_name = filename + "_" + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        fig_list.append(plt.figure())
        plt.subplot(111)
        plt.title(plot_title)
        plt.plot(hys.soc, hys.dv_hys, color='red', label='dv_hys vs soc')
        plt.legend(loc=2)
        fig_file_name = filename + "_" + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        return fig_list, fig_files


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
        fig_list = []
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time

        fig_list, fig_files = overall(hys.saved, filename, fig_files, plot_title=plot_title, fig_list=fig_list)

        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', save_pdf_path='figures')
        cleanup_fig_files(fig_files)
        plt.show()

    main()
