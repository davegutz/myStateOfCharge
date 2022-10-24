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
def resample(data, spacing, factor, specials=None):
    # Check inputs
    if type(factor) is not int or factor < 1:
        raise Exception('factor=', factor, 'must be positive integer > 0')

    # Factors
    n = len(data)
    new_n = (n - 1) * factor + 1
    rat = 1. / float(factor)
    true_spacing = spacing * rat

    # Index for new array
    irec = np.zeros(new_n)
    for i in range(new_n):
        irec[i] = i + 1

    # New array
    recon = np.array(irec, dtype=[('i', 'i4')])
    for var_name, typ in data.dtype.descr:
        var = data[var_name]
        order = 1
        if specials is not None:
            for spec in specials:
                if spec[0] == var_name:
                    order = spec[1]
        new_var = []
        num = 0
        for i in range(n-1):
            base = var[i]
            ext = var[i+1]
            if typ == '<f8':
                for j in range(factor):
                    val = base + (ext-base)*float(order*j)*rat
                    new_var.append(val)
            else:
                for j in range(factor):
                    val = int(base + (ext-base)*float(order*j)*rat)
                    num += 1
                    new_var.append(val)
        num += 1
        val = ext
        new_var.append(val)
        if typ == '<f8':
            recon = rf.rec_append_fields(recon, var_name, np.array(new_var, dtype=float))
        else:
            recon = rf.rec_append_fields(recon, var_name, np.array(new_var, dtype=int))

    return recon, true_spacing


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
        recon = resample(raw, T_raw, 2, specials=[('falw', 0)])

        print("recon")
        print(recon)

main()
