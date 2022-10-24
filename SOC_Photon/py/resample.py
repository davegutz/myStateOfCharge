# Resample a numpy array (add points by interpolation)
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
__date__ = '$Date: 2022/10/23 03:44:00 $'

# https://www.programcreek.com/python/example/95947/scipy.ndimage.interpolation.zoom  example 22

import numpy as np
import numpy.lib.recfunctions as rf


# Unix-like cat function
# e.g. > cat('out', ['in0', 'in1'], path_to_in='./')
def cat(out_file_name, in_file_names, in_path='./', out_path='./'):
    with open(out_path + './' + out_file_name, 'w') as out_file:
        for in_file_name in in_file_names:
            with open(in_path + '/' + in_file_name) as in_file:
                for line in in_file:
                    if line.strip():
                        out_file.write(line)


# Limited capability resample.   Interpolates floating point (foh) or holds value (zoh) according to order input
def resample(data, T_data, T_resamp, specials=None, time_var=None, make_time_float=True):

    # Factors
    factor = int(T_data / T_resamp)
    n = len(data)
    if time_var is None:
        new_n = (n - 1) * factor + 1
    else:  # detect time factors
        time = data[time_var]
        time_type = data[time_var].dtype
        start = time[0]
        end = time[-1]
        span = end - start
        new_spacing = T_data / float(factor)
        new_n = int(span / new_spacing) + 1
        new_time = np.zeros(new_n)
        for i in range(new_n):
            new_time[i] = data[time_var][0] + float(i) * T_resamp
    rat = 1. / float(factor)

    # Index for new array
    irec = np.zeros(new_n)
    for i in range(new_n):
        irec[i] = i + 1

    # New array
    if time_var is None:
        recon = np.array(irec, dtype=[('i', 'i4')])
    else:
        if make_time_float:
            recon = np.array(new_time, dtype=[(time_var, 'float64')])
        else:
            recon = np.array(new_time, dtype=[(time_var, time_type)])
    for var_name, typ in data.dtype.descr:
        var = data[var_name]
        order = 1
        if specials is not None:
            for spec in specials:
                if spec[0] == var_name:
                    order = spec[1]
                    if type(order) is not int or order < 0 or order > 1:
                        raise Exception('order=', order, 'from', spec, 'must be 0 or 1')

        # Add interpolated values
        if time_var is None:
            new_var = []
            for i in range(n-1):
                base = var[i]
                ext = var[i+1]
                if typ == '<f8':
                    for j in range(factor):
                        val = base + (ext-base)*float(order*j)*rat
                        new_var.append(val)
                else:
                    for j in range(factor):
                        val = int(round(base + (ext-base)*float(order*j)*rat))
                        new_var.append(val)
            val = ext
            new_var.append(val)
        else:
            new_var = []
            num = 0
            for i in range(n-1):
                time_base = float(data[time_var][i])
                time_ext = float(data[time_var][i+1])
                dtime = time_ext - time_base
                base = float(var[i])
                ext = float(var[i+1])
                if typ == '<f8':
                    while num < new_n-1:
                        time = new_time[num]
                        val = base + (ext-base) * (time-time_base) / dtime
                        new_var.append(val)
                        num += 1
                else:
                    while num < new_n-1:
                        time = new_time[num]
                        val = int(round(base + (ext-base) * (time-time_base) / dtime))
                        new_var.append(val)
                        num += 1
            val = ext
            new_var.append(val)
            num += 1

        if var_name != time_var:
            if typ == '<f8':
                recon = rf.rec_append_fields(recon, var_name, np.array(new_var, dtype=float))
            else:
                recon = rf.rec_append_fields(recon, var_name, np.array(new_var, dtype=int))

    return recon


if __name__ == '__main__':
    from DataOverModel import write_clean_file


    def main():
        input_files = ['hist v20220926 20221006.txt', 'hist v20220926 20221006a.txt', 'hist v20220926 20221008.txt',
                       'hist v20220926 20221010.txt', 'hist v20220926 20221011.txt']
        exclusions = [(0, 1665334404)]  # before faults
        # exclusions = None
        data_file = 'data20220926.txt'
        path_to_data = '../dataReduction'
        path_to_temp = '../dataReduction/temp'
        cols = ('time', 'Tb', 'Vb', 'Ib', 'soc', 'soc_ekf', 'Voc_dyn', 'Voc_stat', 'tweak_sclr_amp',
                'tweak_sclr_noa', 'falw')

        # cat files
        cat(data_file, input_files, in_path=path_to_data, out_path=path_to_temp)

        # Load mon v4 (old)
        data_file_clean = write_clean_file(data_file, type_='', title_key='hist', unit_key='unit_h',
                                           path_to_data=path_to_temp, path_to_temp=path_to_temp,
                                           comment_str='---')
        if not data_file_clean:
            print("data from", data_file, "empty after loading")
            exit(1)

        # load
        raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols, dtype=None,
                            encoding=None).view(np.recarray)

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
        T = T_raw/1.
        recon = resample(data=raw, T_data=T_raw, T_resamp=T, specials=[('falw', 0)], time_var='time')

        print("recon")
        print(recon)
        print("len raw", len(raw), "len recon", len(recon))
        print("raw time range", raw['time'][0], '-', raw['time'][-1])
        print("recon time range", recon['time'][0], '-', recon['time'][-1])

main()
