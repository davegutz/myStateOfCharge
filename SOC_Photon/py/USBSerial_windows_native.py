import socket

#adapter_addr = '00:14:03:05:59:78'  # for the HC-06
# adapter_addr = '54:27:1e:f4:74:73'  # Broadcom 20702 Bluetooth 4.0 Adapter Properties
import time
import serial
import io
import time
import os

# MAC = '00:14:03:05:59:90'
# port = 3
# buf_size = 1024
#
# s = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
# s.connect((MAC, 1))
# r = s.makefile("r")
# count = 0
# try:
#     while True and count<30:
#         count += 1
#         time.sleep(0.8)
#         data_r = r.readline()
#         if data_r:
#             print(count, data_r)
# except Exception as err:
#     print("Something went wrong: ", err)
#     s.close()

s = serial.Serial(port='COM4', baudrate=115200,  bytesize=8, parity='N', stopbits=1, timeout=None, xonxoff=0, rtscts=0)
var_str = "unit,               hm,                  cTime,       dt,       sat,sel,mod,  Tb,  Vb,  Ib,        Vsat,Vdyn,Voc,Voc_ekf,     y_ekf,    soc_m,soc_ekf,soc,soc_wt,"
vars = var_str.replace(" ", "").split(',')

count = 0
try:
    while count<5:
        count += 1
        time.sleep(0.8)
        data_r = s.readline().decode().rstrip()
        if data_r.__contains__("pro_"):
            list_r = data_r.split(',')
            Ib = float(list_r[9])
            print(count, Ib)
except:
    print('exit')
    exit(1)
