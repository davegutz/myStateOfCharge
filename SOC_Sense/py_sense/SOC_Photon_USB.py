# SOC_Photon_USBSerial:  stream from USB serial and plot known data structure
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

# from platform import system
# if system() == 'Linux':
from kivy.utils import platform
if platform == 'linux':
    import kivy
    from usb4a import usb
    from usbserial4a import serial4a
else:
    from serial import Serial
from plot_soc_photon_data import *

# if system() == 'Linux':
if platform == 'linux':
    device_name = '/dev/bus/usb/001/002'
    device = usb.get_usb_device(device_name)
    if not usb.has_usb_permission(device):
        usb.request_usb_permission(device)
    s = serial4a.get_serial_port(device_name, 115200,  8, 'N', 1, timeout=1)
else:
    com_port = 'COM4'
    try:
        s = Serial(port=com_port, baudrate=115200,  bytesize=8, parity='N', stopbits=1, timeout=None, xonxoff=0, rtscts=0)
    except IOError:
        print('\n\n*************       Have you plugged in USB?      **********************\n\n')
        exit(1)
key = 'pro_'

plot_soc_photon_data(s, key)
