# SOC_Photon_BLESerial:  stream from bluetooth and plot known data structure
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

import socket
from plot_SOC_Photon_data import *

MAC = '00:14:03:05:59:90'; key = 'pro_';
# MAC = '00:14:03:05:59:78'; key = 'soc0_'; # HC-06 on soc0

s = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
s.connect((MAC, 1))
r = s.makefile("r")
plot_SOC_Photon_data(r, key)
