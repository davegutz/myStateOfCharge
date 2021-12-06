# Test_plot.py
# Copyright (C) 2021 Dave Gutz
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

"""Develop structure for simulation plots, including overplot capability and saving/loading to files."""

import numpy as np
import pandas as pd


class ClassWithPanda:
    def __init__(self, dt_=None, name=None, mult=1):
        self.i_call = 0
        self.var1 = 0.
        self.var2 = 0
        self.var3 = True
        self.var4 = 'VAR4'
        self.dt = dt_
        self.time = 0.
        self.name = name
        self.mult = mult
        d = {"var1": self.var1, "var2": self.var2, "var3": self.var3, "var4": self.var4}
        self.df = pd.DataFrame(d, index=[self.time])

    def calc(self):
        self.i_call += 1
        self.var1 = -0.5*self.mult + 0.5*self.mult*self.i_call**2 + 0.00002*self.mult*self.i_call**3
        self.var2 = self.i_call * 3 * self.mult
        self.var3 = (self.var2 % 2) != 0

    def save(self, time_=None):
        if time_ is None:
            self.time = self.dt * self.i_call
        d = {"var1": self.var1, "var2": self.var2, "var3": self.var3, "var4": self.var4}
        df = pd.DataFrame(d, index=[self.time])
        self.df = self.df.append(df)


if __name__ == '__main__':
    import os
    from datetime import datetime
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt


    def main():
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        time = 0.
        ii_pass = 0
        dt = 0.1
        time_end = 10.
        process1 = ClassWithPanda(name='process 1', mult=1)
        process2 = ClassWithPanda(name='process 2', mult=2)

        # Executive tasks
        t = np.arange(0, time_end + dt, dt)

        # Loop time
        for i in range(len(t)):
            time = t[i]
            process1.calc()
            process2.calc()
            process1.save(time)
            process2.save(time)

        print(process1.df)

    main()
