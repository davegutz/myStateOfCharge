import socket

#adapter_addr = '00:14:03:05:59:78'  # for the HC-06
# adapter_addr = '54:27:1e:f4:74:73'  # Broadcom 20702 Bluetooth 4.0 Adapter Properties
import time

MAC = '00:14:03:05:59:90'
port = 3
buf_size = 1024

s = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
s.connect((MAC, 1))
r = s.makefile("r")
count = 0
try:
    while True and count<30:
        count += 1
        time.sleep(0.8)
        data_r = r.readline().rstrip()
        if data_r:
            print(count, data_r)
except Exception as err:
    print("Something went wrong: ", err)
    s.close()
