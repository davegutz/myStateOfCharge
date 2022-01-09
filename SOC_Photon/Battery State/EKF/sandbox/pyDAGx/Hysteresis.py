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
__date__ = '$Date: 2022/01/08 13:15:02 $'

import numpy as np
from pyDAGx.lookup_table import LookupTable

class Hysteresis():
    # Use variable resistor to create hysteresis from an RC circuit

    def __init__(self, t_dv=None, t_soc=None, t_r=None):
        # Defaults
        if t_dv is None:
            t_dv = [-0.08, -1e-6, 0., 0.08]
        if t_soc is None:
            t_soc = [.1, .5, 1]
        if t_r is None:
            t_r = [1e-7, 0.006, 0.003, 1e-7,
                   1e-7, 0.006, 0.003, 1e-7,
                   1e-7, 0.006, 0.003, 1e-7]
        lut = LookupTable()
        lut.addAxis('x', t_dv)
        lut.addAxis('y', t_soc)
        lut.setValueTable(t_r)
        print(lut.lookup(x=.04, y=.5))

    def init(self):
        self.saved = Saved()  # for plots and prints


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.ib = []
        self.vb = []
        self.vc = []


if __name__ == '__main__':
    import sys
    import doctest
    from datetime import datetime
    from unite_pictures import unite_pictures_into_pdf
    import os

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt

    def overall(hys=Hysteresis(), filename='', fig_files=None, plot_title=None, n_fig=None):
        if fig_files is None:
            fig_files = []

        plt.figure()
        n_fig += 1
        plt.subplot(321)
        plt.title(plot_title)
        # plt.plot(ms.time, ref, color='black', label='curr dmd, A')
        # plt.plot(ms.time, ms.ib, color='green', label='ib')
        # plt.plot(ms.time, ms.irc, color='red', label='I_R_ct')
        # plt.plot(ms.time, ms.icd, color='cyan', label='I_C_dif')
        # plt.plot(ms.time, ms.ird, color='orange', linestyle='--', label='I_R_dif')
        # # plt.plot(ms.time, ms.ib, color='black', linestyle='--', label='Ioc')
        # plt.legend(loc=1)
        # plt.subplot(323)
        # plt.plot(ms.time, ms.vb, color='green', label='Vb')
        # plt.plot(ms.time, ms.vc, color='blue', label='Vc')
        # plt.plot(ms.time, ms.vd, color='red', label='Vd')
        # plt.plot(ms.time, ms.voc_dyn, color='orange', label='Voc_dyn')
        # plt.legend(loc=1)
        # plt.subplot(325)
        # plt.plot(ms.time, ms.vbc_dot, color='green', label='Vbc_dot')
        # plt.plot(ms.time, ms.vcd_dot, color='blue', label='Vcd_dot')
        # plt.legend(loc=1)
        # plt.subplot(322)
        # plt.plot(ms.time, ms.soc, color='red', label='soc')
        # plt.legend(loc=1)
        # plt.subplot(324)
        # plt.plot(ms.time, ms.pow_oc, color='orange', label='Pow_charge')
        # plt.legend(loc=1)
        # plt.subplot(326)
        # plt.plot(ms.soc, ms.voc, color='black', label='voc vs soc')
        # plt.legend(loc=1)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        return n_fig, fig_files


    def main():
        # Setup to run the transients
        dt = 0.1
        # time_end = 700
        time_end = 3500

        hys = Hysteresis()

        # Executive tasks
        t = np.arange(0, time_end + dt, dt)
        current_in_s = []

        # time loop
        for i in range(len(t)):
            if t[i] < 50:
                current_in = 0.
            elif t[i] < 450:
                current_in = -40.
            elif t[i] < 1000:
                current_in = 0.
            elif t[i] < 3000:
                current_in = 40.
            else:
                current_in = 0.
            init_ekf = (t[i] <= 1)

            if init_ekf:
                hys.init()

            # Models
            hys.calculate()


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
        for fig_file in fig_files:
            try:
                os.remove(fig_file)
            except OSError:
                pass
        plt.show()

    main()
