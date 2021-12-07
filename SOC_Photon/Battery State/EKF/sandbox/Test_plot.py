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

"""Develop structure for simulation plots, including over plot capability and saving/loading to files."""
import math
import numpy as np
import pandas as pd
import tracemalloc, os, linecache
from time import perf_counter


def display_top(snapshot, key_type='lineno', limit=3, details=False, msg=""):
    snapshot = snapshot.filter_traces((
        tracemalloc.Filter(False, "<frozen importlib._bootstrap>"),
        tracemalloc.Filter(False, "<unknown>"),
    ))
    top_stats = snapshot.statistics(key_type)

    if details:
        print("Top %s lines" % limit)
        for index, stat in enumerate(top_stats[:limit], 1):
            frame = stat.traceback[0]
            # replace "/path/to/module/file.py" with "module/file.py"
            filename = os.sep.join(frame.filename.split(os.sep)[-2:])
            print("#%s: %s:%s: %.1f KiB"
                  % (index, filename, frame.lineno, stat.size / 1024))
            line = linecache.getline(frame.filename, frame.lineno).strip()
            if line:
                print('    %s' % line)

    other = top_stats[limit:]
    if other:
        size = sum(stat.size for stat in other)
        if details:
            print("%s other: %.1f KiB" % (len(other), size / 1024))
    total = sum(stat.size for stat in top_stats)
    print(msg, "Total allocated size: %.1f KiB" % (total / 1024))


def over_plot(sig, colors=None, title='', t_lim=None, y_lim=None, date_time='', loc=4):
    if t_lim is None:
        t_zooming = False
        x_min = 0
        x_max = sig[0].index[:]
    else:
        t_zooming = True
        x_min = t_lim[0]
        x_max = t_lim[1]

    y_min = math.inf
    y_max = -math.inf
    if y_lim is None:
        y_zooming = False
    else:
        y_zooming = True
        y_min = y_lim[0]
        y_max = y_lim[1]
    n = len(sig)
    # m = len(sig[0])
    # nc = len(colors)

    for i in range(0, n):
        plt.plot(sig[i].index, sig[i].values, colors[i], label=sig[i].name)
        if t_zooming:
            zoom_range = (sig[i].index > t_lim[0]) & (sig[i].index < t_lim[1])
            if y_zooming:
                y_min = min(y_min, sig[i].values[zoom_range].min())
                y_max = max(y_max, sig[i].values[zoom_range].max())
    plt.title(title + '  ' + date_time)
    plt.grid()
    plt.legend(loc=loc)
    if t_zooming:
        plt.xlim(x_min, x_max)
    if y_zooming:
        plt.ylim(y_min, y_max)


def overplot(sig, sig_names, colors=None, title='', t_lim=None, y_lim=None, date_time='', loc=4):
    if t_lim is None:
        t_zooming = False
        x_min = 0
        x_max = sig[0][:]
    else:
        t_zooming = True
        x_min = t_lim[0]
        x_max = t_lim[1]

    y_min = math.inf
    y_max = -math.inf
    if y_lim is None:
        y_zooming = False
    else:
        y_zooming = True
        y_min = y_lim[0]
        y_max = y_lim[1]
    n = len(sig)
    # m = len(sig[0])
    # nc = len(colors)

    for i in range(0, n):
        plt.plot(sig[i][0], sig[i][1], colors[i], label=sig_names[i])
        if t_zooming:
            zoom_range = (sig[i][0] > t_lim[0]) & (sig[i][0] < t_lim[1])
            if y_zooming & (max(zoom_range) == True):
                y_min = min(y_min, min((sig[i][1])[zoom_range]))
                y_max = max(y_max, max((sig[i][1])[zoom_range]))
    plt.title(title + '  ' + date_time)
    plt.grid()
    plt.legend(loc=loc)
    if t_zooming:
        plt.xlim(x_min, x_max)
    if y_zooming:
        plt.ylim(y_min, y_max)


class ClassWithPanda:
    def __init__(self, dt_=None, name=None, multi=1):
        self.i_call = 0
        self.var1 = 0.
        self.var2 = 0
        self.var3 = True
        self.var4 = 'VAR4'
        self.dt = dt_
        self.time = 0.
        self.name = name
        self.multi = multi
        d = {name+".var1": self.var1, name+".var2": self.var2, name+".var3": self.var3, name+".var4": self.var4}
        self.df = pd.DataFrame(d, index=[self.time])
        self.np_list = []
        self.np_arr = np.array([])

    def calc(self):
        self.i_call += 1
        self.var1 = -0.5*self.multi + 0.5*self.multi*self.i_call**2 + 0.00002*self.multi*self.i_call**3
        self.var2 = self.i_call * 3 * self.multi
        self.var3 = (self.var2 % 2) != 0

    def save(self, time_=None):
        if time_ is None:
            self.time = self.dt * self.i_call
        else:
            self.time = time_
        d = {self.name+".var1": self.var1, self.name+".var2": self.var2, self.name+".var3": self.var3, self.name+".var4": self.var4}
        df = pd.DataFrame(d, index=[self.time])
        self.df = self.df.append(df)

    def save_np(self, time_=None):
        if time_ is None:
            self.time = self.dt * self.i_call
        else:
            self.time = time_
        self.np_list.append([self.time, self.var1, self.var2, self.var3])

    def save_arr(self):
        # print('list=', self.np_list)
        self.np_arr = np.array(self.np_list)
        # print('arr=', self.np_arr)


if __name__ == '__main__':
    # import os
    from datetime import datetime
    import sys
    import doctest
    from matplotlib import pyplot as plt

    doctest.testmod(sys.modules['__main__'])


    def main():
        tracemalloc.start()
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        dt = 0.1
        time_end = 10.

        # Calculate only
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='Before calc only loop:  ')
        start = perf_counter()
        process1 = ClassWithPanda(name='proc1', multi=1)
        process2 = ClassWithPanda(name='proc2', multi=2)
        # Loop time
        t = np.arange(0, time_end + dt, dt)
        for i in range(len(t)):
            time = t[i]
            process1.calc()
            process2.calc()
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After calc only loop:  ')
        end = perf_counter()
        print('   process time=', (end - start) / float(len(t)), 'seconds per iteration')

        # Collect np data
        print('')
        process1 = ClassWithPanda(name='proc1', multi=1)
        process2 = ClassWithPanda(name='proc2', multi=2)
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='Before collect np data loop:  ')
        start = perf_counter()
        # Loop time
        t = np.arange(0, time_end + dt, dt)
        for i in range(len(t)):
            time = t[i]
            process1.calc()
            process2.calc()
            process1.save_np(time)
            process2.save_np(time)
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After collect np data only loop:  ')
        end = perf_counter()
        print('   process time=', (end - start) / float(len(t)), 'seconds per iteration')

        # Collect pandas data
        print('')
        process1 = ClassWithPanda(name='proc1', multi=1)
        process2 = ClassWithPanda(name='proc2', multi=2)
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='Before collect pandas data only loop:  ')
        start = perf_counter()
        # Loop time
        t = np.arange(0, time_end + dt, dt)
        for i in range(len(t)):
            time = t[i]
            process1.calc()
            process2.calc()
            process1.save(time)
            process2.save(time)
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After collect pandas data only loop:  ')
        end = perf_counter()
        print('   process time=', (end - start) / float(len(t)), 'seconds per iteration')

        # Collect pandas data and plot
        print('')
        process1 = ClassWithPanda(name='proc1', multi=1)
        process2 = ClassWithPanda(name='proc2', multi=2)
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='Before collect pandas data for plot loop:  ')
        start = perf_counter()
        # Loop time
        t = np.arange(0, time_end + dt, dt)
        for i in range(len(t)):
            time = t[i]
            process1.calc()
            process2.calc()
            process1.save(time)
            process2.save(time)
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After collect pandas data for plot loop:  ')
        end = perf_counter()
        print('   process time=', (end - start) / float(len(t)), 'seconds per iteration')
        # Plots
        n_fig = 0
        fig_files = []
        plt.figure()
        n_fig += 1
        plt.subplot(221)
        over_plot([process1.df['proc1.var1'], process2.df['proc2.var1']], colors=['red', 'blue'], loc=2,
                  t_lim=[5, 7], y_lim=[1000, 6000])
        plt.title(filename + '  ' + date_time)
        plt.subplot(222)
        over_plot([process1.df['proc1.var2'], process2.df['proc2.var2']], colors=['red', 'blue'], loc=2)
        plt.subplot(223)
        over_plot([process1.df['proc1.var3'], process2.df['proc2.var3']], colors=['red', 'blue'], loc=2)
        fig_file_name = filename + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")
        # plt.show()
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After pandas over plot:  ')
        plt.close()
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After close pandas over plot:  ')
        plt.figure()
        n_fig += 1
        plt.subplot(221)
        plt.title(filename + '  ' + date_time)
        plt.plot(process1.df['proc1.var1'], color='black', label='proc1.var1')
        plt.plot(process2.df['proc2.var1'], color='red', label='proc2.var1')
        plt.legend(loc=2)
        plt.subplot(222)
        plt.plot(process1.df['proc1.var2'], color='black', label='proc1.var2')
        plt.plot(process2.df['proc2.var2'], color='red', label='proc2.var2')
        plt.legend(loc=2)
        plt.subplot(223)
        plt.plot(process1.df['proc1.var3'], color='black', label='proc1.var3')
        plt.plot(process2.df['proc2.var3'], color='red', label='proc2.var3')
        plt.legend(loc=2)
        fig_file_name = filename + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After pandas plot:  ')
        # plt.show()
        plt.close()
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After close pandas over plot:  ')

        # Collect np data and plot
        print('')
        process1 = ClassWithPanda(name='proc1', multi=1)
        process2 = ClassWithPanda(name='proc2', multi=2)
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='Before collect np data for plot loop:  ')
        start = perf_counter()
        # Loop time
        t = np.arange(0, time_end + dt, dt)
        for i in range(len(t)):
            time = t[i]
            process1.calc()
            process2.calc()
            process1.save_np(time)
            process2.save_np(time)
        process1.save_arr()
        process2.save_arr()
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After collect np data for plot loop:  ')
        end = perf_counter()
        print('   process time=', (end - start) / float(len(t)), 'seconds per iteration')
        # Plots
        n_fig = 0
        fig_files = []
        plt.figure()
        n_fig += 1
        plt.subplot(221)
        overplot([[process1.np_arr[:,0], process1.np_arr[:,1]], [process2.np_arr[:,0], process2.np_arr[:,1]]],
                 sig_names=['proc1.var1', 'proc2.var1'],
                 colors=['red', 'blue'], loc=2,
                 t_lim=[5, 7], y_lim=[1000, 6000])
        plt.title(filename + '  ' + date_time)
        plt.subplot(222)
        overplot([[process1.np_arr[:,0], process1.np_arr[:,2]], [process2.np_arr[:,0], process2.np_arr[:,2]]],
                 sig_names=['proc1.var2', 'proc2.var2'],
                 colors=['red', 'blue'], loc=2)
        plt.subplot(223)
        overplot([[process1.np_arr[:,0], process1.np_arr[:,3]], [process2.np_arr[:,0], process2.np_arr[:,3]]],
                 sig_names=['proc1.var3', 'proc2.var3'],
                 colors=['red', 'blue'], loc=2)
        fig_file_name = filename + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")
        # plt.show()
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After np over plot:  ')
        plt.close()
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After np close over plot:  ')
        plt.figure()
        n_fig += 1
        # plt.subplot(221)
        # plt.title(filename + '  ' + date_time)
        # plt.plot(process1.df['proc1.var1'], color='black', label='proc1.var1')
        # plt.plot(process2.df['proc2.var1'], color='red', label='proc2.var1')
        # plt.legend(loc=2)
        # plt.subplot(222)
        # plt.plot(process1.df['proc1.var2'], color='black', label='proc1.var2')
        # plt.plot(process2.df['proc2.var2'], color='red', label='proc2.var2')
        # plt.legend(loc=2)
        # plt.subplot(223)
        # plt.plot(process1.df['proc1.var3'], color='black', label='proc1.var3')
        # plt.plot(process2.df['proc2.var3'], color='red', label='proc2.var3')
        # plt.legend(loc=2)
        fig_file_name = filename + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After np plot:  ')
        # plt.show()
        plt.close()
        snapshot = tracemalloc.take_snapshot()
        display_top(snapshot, msg='After close np over plot:  ')


    main()
