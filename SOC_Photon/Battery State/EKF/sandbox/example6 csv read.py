# demo of csv file read
import matplotlib.pyplot as plt
import numpy as np


data_file = '../../../dataReduction/data_proto.csv'

try:
    data
except:
    #cols = ('t', 'vshunt_int', '0_int', '1_int', 'v0', 'v1', 'vshunt', 'Ishunt', 'delim', 'vshunt_amp_int', '0_amp_int', '1_amp_int', 'v0_amp', 'v1_amp', 'Vshunt_amp', 'Ishunt_amp', 'T')
    cols = ('t', 'Ishunt')
    data = np.genfromtxt(data_file, delimiter=',', names=True, usecols=cols, dtype=None, encoding=None).view(np.recarray)

plt.plot(data.t, data.Ishunt)
plt.show()
