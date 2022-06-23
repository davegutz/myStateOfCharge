# SOC_Photon_USBSerial:  stream from USB serial and plot known data structure
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

from platform import system
if system() == 'android':
    # from usb4a import usb
    from usbserial4a import serial4a
else:
    from serial import Serial
from plot_SOC_Photon_data import *

com_port = 'COM4'
key = 'pro_'

if system() == 'android':
    s = serial4a.get_serial_port(port=com_port, baudrate=115200,  bytesize=8, parity='N', stopbits=1, timeout=None, xonxoff=0, rtscts=0)
else:
    s = Serial(port=com_port, baudrate=115200,  bytesize=8, parity='N', stopbits=1, timeout=None, xonxoff=0, rtscts=0)

plot_SOC_Photon_data(s, key)
