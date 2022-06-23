import serial
import time
import numpy as np
from pylive import liven_plotter
import matplotlib.pyplot as plt
from BLESerial_windows_native import plot_SOC_Photon_data

com_port = 'COM4'
key = 'pro_'

s = serial.Serial(port=com_port, baudrate=115200,  bytesize=8, parity='N', stopbits=1, timeout=None, xonxoff=0, rtscts=0)
plot_SOC_Photon_data(s)
