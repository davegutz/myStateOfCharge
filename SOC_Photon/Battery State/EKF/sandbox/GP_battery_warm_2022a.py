# GP_battery_warm - general purpose battery heating model
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

"""Define a general purpose battery heating model with odd number
of slices to place bulk temperature in center.  Sensor on top.   Heater
at bottom represented by W input.   Ambient temperature temp_c"""

import numpy as np
import BatteryHeat
from BatteryHeat import BatteryHeat, overall
from unite_pictures import unite_pictures_into_pdf
import os
from datetime import datetime

if __name__ == '__main__':
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt
    import book_format
    from pyDAGx import myTables

    book_format.set_style()


    def main():
        # Transient  inputs
        # Controller temp ref - time inputs
        t_x_R = [0.00, 0.99, 1.00]  # hours
        t_R = [  20.0, 20.0, 20.0]  # deg C
        # Ambient temperature - time inputs
        t_x_T = [0.00,  0.99, 1.00]  # hours
        t_T = [  -10.0, -10.0, -10.0]  # deg C
        # Heat, W inputs
        W_max = 36.
        time_end = 48.
        T_Init = t_T[0]
        T_Ref = T_Init

        # Setup
        dt = 0.1  # hours
        lut_R = myTables.TableInterp1D(np.array(t_x_R), np.array(t_R))
        lut_T = myTables.TableInterp1D(np.array(t_x_T), np.array(t_T))
        sim = BatteryHeat(temp_c=T_Init, n=5)
        W = 0.
        print('sim:  ', str(sim))

        # time loop
        t = np.arange(0, time_end+dt, dt)
        for i in range(len(t)):
            T_Ref = lut_R.interp(t[i])
            temp_c = lut_T.interp(t[i])
            init = (t[i] <= 1.)

            if init:
                sim.assign_temp_c(temp_c)
                T_Ref = temp_c

            # Control
            if T_Ref > sim.Tns and not init:
            # if T_Ref > sim.Tb and not init:
                    W = W_max
            else:
                W = 0.

            # Models
            sim.calculate(temp_c=temp_c, W=W, dt=dt)

            # Plot stuff
            sim.save(time=t[i], T_ref=T_Ref)

        # Data
        print('sim:  ', str(sim))
        print('T_Ref=', T_Ref)

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time

        n_fig, fig_files = overall(sim.saved, filename, fig_files,
                                   plot_title=plot_title, n_fig=n_fig)

        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='figures')
        for fig_file in fig_files:
            try:
                os.remove(fig_file)
            except OSError:
                pass
        plt.show()

    main()
