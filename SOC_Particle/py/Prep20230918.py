import os
from resample import resample
from DataOverModel import write_clean_file
import numpy as np
import pandas as pd


def data_file(data_file_path, data_file_txt):
    path_to_data = os.path.join(data_file_path, data_file_txt)
    return path_to_data


T = 5.  # Update to interpolate missing data
file0 = 'dv_20230911_soc0p_ch_raw.csv'
file1 = 'putty_2023-09-15T12-08-18.csv'
file2 = 'putty_2023-09-16T16-02-22.csv'
file3 = 'putty_2023-09-16T20-29-21.csv'
file4 = 'putty_2023-09-17T09-27-16.csv'
file5 = 'putty_2023-09-17T16-48-56.csv'

# Make full path file names
temp_file0 = data_file(os.getcwd(), file0)
temp_file1 = data_file(os.getcwd(), file1)
temp_file2 = data_file(os.getcwd(), file2)
temp_file3 = data_file(os.getcwd(), file3)
temp_file4 = data_file(os.getcwd(), file4)
temp_file5 = data_file(os.getcwd(), file5)

# Make clean files
temp_file0_clean = write_clean_file(temp_file0, type_='', hdr_key='unit', unit_key='g20230530')
temp_file1_clean = write_clean_file(temp_file1, type_='', hdr_key='unit', unit_key='g20230530')
temp_file2_clean = write_clean_file(temp_file2, type_='', hdr_key='unit', unit_key='g20230530')
temp_file3_clean = write_clean_file(temp_file3, type_='', hdr_key='unit', unit_key='g20230530')
temp_file4_clean = write_clean_file(temp_file4, type_='', hdr_key='unit', unit_key='g20230530')
temp_file5_clean = write_clean_file(temp_file5, type_='', hdr_key='unit', unit_key='g20230530')

# Load all clean
t0_raw = np.genfromtxt(temp_file0_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
t1_raw = np.genfromtxt(temp_file1_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
t2_raw = np.genfromtxt(temp_file2_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
t3_raw = np.genfromtxt(temp_file3_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
t4_raw = np.genfromtxt(temp_file4_clean, delimiter=',', names=True, dtype=float).view(np.recarray)
t5_raw = np.genfromtxt(temp_file5_clean, delimiter=',', names=True, dtype=float).view(np.recarray)

# Resample transitions
t01_raw = t1_raw[0:2].copy()
t01_raw[0] = t0_raw[-1].copy()
t01_raw[1] = t1_raw[0].copy()
t12_raw = t2_raw[0:2].copy()
t12_raw[0] = t1_raw[-1].copy()
t12_raw[1] = t2_raw[0].copy()
t23_raw = t3_raw[0:2].copy()
t23_raw[0] = t2_raw[-1].copy()
t23_raw[1] = t3_raw[0].copy()
t34_raw = t4_raw[0:2].copy()
t34_raw[0] = t3_raw[-1].copy()
t34_raw[1] = t4_raw[0].copy()
t45_raw = t5_raw[0:2].copy()
t45_raw[0] = t4_raw[-1].copy()
t45_raw[1] = t5_raw[0].copy()
t01_resamp = resample(data=t01_raw, dt_resamp=T, time_var='cTime', specials=[('unit', -1), ('dt', 0), ('chm', 0), ('sel', 0), ('mod', 0), ('bmso', 0), ('sat', 0)])
t01_resamp[:]['unit'] = t01_resamp[0]['unit']
t01_resamp = t01_resamp[1:-1]
t12_resamp = resample(data=t12_raw, dt_resamp=T, time_var='cTime', specials=[('unit', -1), ('dt', 0), ('chm', 0), ('sel', 0), ('mod', 0), ('bmso', 0), ('sat', 0)])
t12_resamp[:]['unit'] = t12_resamp[0]['unit']
t12_resamp = t12_resamp[1:-1]
t23_resamp = resample(data=t23_raw, dt_resamp=T, time_var='cTime', specials=[('unit', -1), ('dt', 0), ('chm', 0), ('sel', 0), ('mod', 0), ('bmso', 0), ('sat', 0)])
t23_resamp[:]['unit'] = t23_resamp[0]['unit']
t23_resamp = t23_resamp[1:-1]
t34_resamp = resample(data=t34_raw, dt_resamp=T, time_var='cTime', specials=[('unit', -1), ('dt', 0), ('chm', 0), ('sel', 0), ('mod', 0), ('bmso', 0), ('sat', 0)])
t34_resamp[:]['unit'] = t34_resamp[0]['unit']
t34_resamp = t34_resamp[1:-1]
t45_resamp = resample(data=t45_raw, dt_resamp=T, time_var='cTime', specials=[('unit', -1), ('dt', 0), ('chm', 0), ('sel', 0), ('mod', 0), ('bmso', 0), ('sat', 0)])
t45_resamp[:]['unit'] = t45_resamp[0]['unit']
t45_resamp = t45_resamp[1:-1]

# Join them all and save to file for subsequent run by CompareTensorData.py
t_clean = np.hstack((t0_raw, t01_resamp, t1_raw, t12_resamp, t2_raw, t23_resamp, t3_raw, t34_resamp, t4_raw, t45_resamp, t5_raw))
# t_clean = np.hstack((t0_raw, t01_resamp, t1_raw))
# t_clean = np.hstack((t0_raw, t01_resamp))
(path, basename) = os.path.split(temp_file5)
path_to_temp = path + '/temp'
if not os.path.isdir(path_to_temp):
    os.mkdir(path_to_temp)
join_save = path_to_temp + '/' + basename.replace('.csv', '_join' + '.csv', 1)
pd.DataFrame(t_clean).to_csv(join_save, index_label='index')
