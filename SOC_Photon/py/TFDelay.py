# Delay class debounce logic
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
__date__ = '$Date: 2022/06/02 13:15:02 $'

import numpy as np


class TFDelay:
    # Use variable resistor to create tfdteresis from an RC circuit

    def __init__(self, in_=False, t_true=0., t_false=0., dt=0.1):
        # Defaults
        self.timer = 0.
        self.nt = int(max(round(t_true/dt)+1, 0))
        self.nf = int(max(round(t_false/dt)+1, 0))
        self.dt = dt
        self.in_ = in_
        self.out = self.in_
        if t_true == 0.0:
            self.nt = 0
        if t_false == 0.0:
            self.nf = 0
        if in_:
            self.timer = self.nf
        else:
            self.time = -self.nt
        self.saved = Saved()

    def calculate1(self, in_):
        if self.timer >= 0.:
            if in_:
                self.timer = self.nf
            else:
                self.timer -= 1
                if self.timer < 0:
                    self.timer = -self.nt
        else:
            if not in_:
                self.timer = -self.nt
            else:
                self.timer += 1
                if self.timer >= 0.:
                    self.timer = self.nf
        out = self.timer > 0.
        return out

    def calculate2(self, in_, reset):
        if reset:
            if in_:
                self.timer = self.nf
            else:
                self.timer = -self.nt
            out = in_
        else:
            out = self.calculate1(in_)
        return out

    def calculate3(self, in_, t_true, t_false):
        self.nt = int(max(round(t_true / self.dt), 0))
        self.nf = int(max(round(t_false / self.dt), 0))
        return self.calculate1(in_)

    def calculate4t(self, in_, t_true, t_false, dt):
        self.dt = dt
        self.nt = int(max(round(t_true / self.dt), 0))
        self.nf = int(max(round(t_false / self.dt), 0))
        return self.calculate1(in_)

    def calculate4r(self, in_, t_true, t_false, reset):
        if reset > 0:
            if in_:
                self.timer = self.nf
            else:
                self.timer = -self.nt
        return self.calculate3(in_, t_true, t_false)

    def calculate(self, in_, t_true, t_false, dt, reset):
        self.in_ = in_
        if reset:
            if self.in_:
                self.timer = self.nf
            else:
                self.timer = -self.nt
        self.out = self.calculate4t(self.in_, t_true, t_false, dt)
        return self.out

    def save(self, time):
        self.saved.time.append(time)
        self.saved.timer.append(self.timer)
        self.saved.in_.append(self.in_)
        self.saved.nt.append(self.nt)
        self.saved.nf.append(self.nf)
        self.saved.dt.append(self.dt)
        self.saved.out.append(self.out)

    def state(self):
        return self.timer > 0.


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.in_ = []
        self.out = []
        self.timer = []
        self.nf = []
        self.nt = []
        self.dt = []


if __name__ == '__main__':
    import sys
    import doctest
    from datetime import datetime
    from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt


    def overall(tfd=TFDelay().saved, filename='', fig_files=None, plot_title=None, n_fig=None):
        if fig_files is None:
            fig_files = []

        plt.figure()
        n_fig += 1
        plt.subplot(211)
        plt.title(plot_title)
        plt.plot(tfd.time, tfd.timer, color='red', linestyle='-', label='timer')
        plt.plot(tfd.time, tfd.nf, color='blue', linestyle='--', label='nf')
        plt.plot(tfd.time, tfd.nt, color='green', linestyle='-.', label='nt')
        plt.legend(loc=3)
        plt.subplot(212)
        plt.plot(tfd.time, tfd.in_, color='red', linestyle='-', label='in')
        plt.plot(tfd.time, tfd.out, color='blue', linestyle='--', label='out')
        plt.legend(loc=3)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        return n_fig, fig_files


    def main():
        # Setup to run the transients
        dt = 10.
        time_end = 100.

        tfd = TFDelay(in_=False, t_true=8., t_false=4., dt=dt)

        # Executive tasks
        t = np.arange(0, time_end + dt, dt)
        inp_s = []

        # time loop
        for i in range(len(t)):
            init_tfd = (t[i] <= 10)
            if t[i]>10. and t[i]<60.:
                inp = True
            else:
                inp = False

            # Models
            tfd.calculate(inp, 8., 4., dt, init_tfd)

            # Plot stuff
            inp_s.append(inp)
            tfd.save(t[i])

        # Data
        print('tfd:  ', str(tfd))

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time

        n_fig, fig_files = overall(tfd.saved, filename, fig_files, plot_title=plot_title, n_fig=n_fig)

        # unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='figures')
        # cleanup_fig_files(fig_files)
        plt.show()

    main()
