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
        t_x_T = [0.0,  0.5, 6.0, 7.0, 8.0, 9.0, 14.0, 15.0, 17.0, 24.0, 26.0]  # hours
        t_T = [  1.7,  1.7, 5.6, 2.8, 2.2, 2.8, -0.6,  0.0, -2.2,  2.8,  8.3]  # deg C
        # Measurement for reference
        t_x_M = [0.,   1.5,  2.,   2.5,  8.,   9.5,  10.5, 13.5, 17.5, 23.,  23.5, 26. ]  # hours
        t_M = [  21.8, 27.1, 26.6, 25.1, 17.6, 17.3, 17.8, 19.6, 20.4, 18.9, 20.2, 22.2  ]  # deg C
        # Heat, W inputs
        t_x_W = [0.0, 5.0, 5.5,  23.5, 24.0]  # hours
        t_W = [  0.0, 0.0, 36.4, 36.4, 0.0]  # Watts
        s_W = 1.0  # scalar on W lookup
        W_max = 36.4
        matching = True
        if matching:
            time_end = 28.
            T_Init = t_M[0]
        else:
            time_end = 48.
            T_Init = 0.
        T_Ref = t_R[0]

        # Setup
        dt = 0.1  # hours
        lut_R = myTables.TableInterp1D(np.array(t_x_R), np.array(t_R))
        lut_T = myTables.TableInterp1D(np.array(t_x_T), np.array(t_T))
        lut_W = myTables.TableInterp1D(np.array(t_x_W), np.array(t_W))
        lut_M = myTables.TableInterp1D(np.array(t_x_M), np.array(t_M))
        nTi = 5
        sim = BatteryHeat(n=nTi, temp_c=T_Init, hi0=1., hij=0.9)
        if matching:
            Ti_init = [90, 80, 70, 60, 32]
        else:
            Ti_init = [0., 0., 0., 0., 0.]
        sim.assign_Ti(Ti_init)
        W = 0.
        print('sim:  ', str(sim))

        # time loop
        t = np.arange(0, time_end+dt, dt)
        for i in range(len(t)):
            T_Ref = lut_R.interp(t[i])
            temp_c = lut_T.interp(t[i])
            if matching:
                Tbm = lut_M.interp(t[i])
            else:
                Tbm = 0
            init = (t[i] <= 0.5)

            if init:
                sim.assign_temp_c(Tbm)
                sim.assign_Ti(Ti_init)

            # Control
            if matching:
                W = lut_W.interp(t[i])*s_W
            else:
                if T_Ref > sim.Tns and not init:
                    W = W_max
                else:
                    W = 0.

            # Models
            sim.calculate(temp_c=temp_c, W=W, dt=dt)

            # Plot stuff
            sim.save(time=t[i], T_ref=T_Ref, T_bm=Tbm)

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
