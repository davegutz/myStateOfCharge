# Resample a numpy array (add points by interpolation)
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
__date__ = '$Date: 2022/10/23 03:44:00 $'

# https://www.programcreek.com/python/example/95947/scipy.ndimage.interpolation.zoom  example 22

import numpy as np
import numpy.lib.recfunctions as rf
from Util import cat


# Limited capability resample.   Interpolates floating point (foh) or holds value (zoh) according to order input
def resample(data, dt_resamp, time_var, specials=None, make_time_float=True):

    # Factors
    n = len(data)
    time = data[time_var]
    time_type = data[time_var].dtype
    start = float(time[0])
    end = float(time[-1])
    new_time = [start]
    new_n = 0
    print("start", start, "end", end)
    while new_time[new_n] < end:
        new_n += 1
        new_time.append(start + float(new_n) * dt_resamp)
    # print("resample:  new time", new_time)
    print("resample:  start new", new_time[0], "end new", new_time[-1], "len new", len(new_time), "end new - end", new_time[-1]-end, "dt_resamp", dt_resamp)

    # Index for new array
    irec = np.zeros(new_n)
    for i in range(new_n):
        irec[i] = i + 1

    # New array
    if make_time_float:
        resamp = np.array(new_time, dtype=[(time_var, 'float64')])
    else:
        resamp = np.array(new_time, dtype=[(time_var, time_type)])
    for var_name, typ in data.dtype.descr:
        var = data[var_name]

        # Verify order
        order = 1
        if specials is not None:
            for spec in specials:
                if spec[0] == var_name:
                    order = spec[1]
                    if type(order) is not int or order < -1 or order > 1:
                        raise Exception('order=', order, 'from', spec, 'must be -1, 0 or 1')

        # Add interpolated values
        new_var = []
        num = 0
        for i in range(n-1):
            time_base = float(data[time_var][i])
            time_ext = float(data[time_var][i+1])
            dtime = time_ext - time_base
            if order >= 0:
                base = float(var[i])
                ext = float(var[i+1])
                time = time_base
                if typ == '<f8':
                    while time < time_ext and num < new_n:
                        val = base + (ext-base) * (time-time_base) * order / dtime
                        new_var.append(val)
                        num += 1
                        time = new_time[num]
                else:
                    while time < time_ext and num < new_n:
                        val = int(round(base + (ext-base) * (time-time_base) * order / dtime))
                        new_var.append(val)
                        num += 1
                        time = new_time[num]
            else:
                val = var[i]
                ext = var[i+1]
                print('ext', ext)
        new_var.append(ext)
        num += 1

        if var_name != time_var:
            if typ == '<f8':
                resamp = rf.rec_append_fields(resamp, var_name, np.array(new_var, dtype=float))
            elif typ == '<U18' or typ == '<U21':
                resamp = rf.rec_append_fields(resamp, var_name, np.array(new_var, dtype='<U21'))
            else:
                resamp = rf.rec_append_fields(resamp, var_name, np.array(new_var, dtype=int))

    return resamp


if __name__ == '__main__':
    from DataOverModel import write_clean_file


    def main():
        input_files = ['hist v20220926 20221006.txt', 'hist v20220926 20221006a.txt', 'hist v20220926 20221008.txt',
                       'hist v20220926 20221010.txt', 'hist v20220926 20221011.txt']
        exclusions = [(0, 1665334404)]  # before faults
        # exclusions = [(0, 1665518004)]  # small test set for debugging

        # exclusions = None
        data_file = 'data20220926.txt'
        path_to_data = '../dataReduction'
        path_to_temp = '../dataReduction/temp'
        import os
        if not os.path.isdir(path_to_temp):
            os.mkdir(path_to_temp)

        # cat files
        cat(data_file, input_files, in_path=path_to_data, out_path=path_to_temp)

        # Load mon v4 (old)
        data_file_clean = write_clean_file(data_file, type_='', title_key='hist', unit_key='unit_f',
                                           path_to_data=path_to_temp, path_to_temp=path_to_temp,
                                           comment_str='---')
        if not data_file_clean:
            print("data from", data_file, "empty after loading")
            exit(1)

        # load
        raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, dtype=float).view(np.recarray)

        # Sort unique
        raw = np.unique(raw)

        # Rack and stack
        if exclusions:
            for i in range(len(exclusions)):
                test_res0 = np.where(raw.time < exclusions[i][0])
                test_res1 = np.where(raw.time > exclusions[i][1])
                raw = raw[np.hstack((test_res0, test_res1))[0]]

        print("raw data")
        print(raw)

        # Now do the resample
        T_raw = raw.time[1] - raw.time[0]
        T = 0.1
        resamp = resample(data=raw, dt_resamp=T, specials=[('falw', 0)], time_var='time')

        print("resamp")
        print(resamp)
        print("raw time range", raw['time'][0], '-', raw['time'][-1], "length=", len(raw), "dt=", T_raw)
        print("resamp time range", resamp['time'][0], '-', resamp['time'][-1], "length=", len(resamp), "dt=", T)

    main()
