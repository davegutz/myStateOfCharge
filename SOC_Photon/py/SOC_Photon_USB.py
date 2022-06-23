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

import serial
import time
import numpy as np
from pylive import liven_plotter
import matplotlib.pyplot as plt
from SOC_Photon_BLE import plot_SOC_Photon_data

com_port = 'COM4'
key = 'pro_'

s = serial.Serial(port=com_port, baudrate=115200,  bytesize=8, parity='N', stopbits=1, timeout=None, xonxoff=0, rtscts=0)
plot_SOC_Photon_data(s)
