# demo of csv file read
import matplotlib.pyplot as plt
import numpy as np

# make csv file by hand from .ods by opening in openOffice and save as csv
# data_file = '../../../dataReduction/data_proto.csv'
data_file = '../../../dataReduction/20211010_064759.csv'

try:
    data
except:
    #cols = ('t', 'vshunt_int', '0_int', '1_int', 'v0', 'v1', 'vshunt', 'Ishunt', 'delim', 'vshunt_amp_int', '0_amp_int', '1_amp_int', 'v0_amp', 'v1_amp', 'Vshunt_amp', 'Ishunt_amp', 'T')
    cols = ('t', 'Vbatt', 'Ishunt')
    data = np.genfromtxt(data_file, delimiter=',', names=True, usecols=cols, dtype=None, encoding=None).view(np.recarray)

plt.subplot(121); plt.title(data_file)
plt.plot(data.t, data.Ishunt)
plt.subplot(122)
plt.plot(data.t, data.Vbatt)
plt.show()
