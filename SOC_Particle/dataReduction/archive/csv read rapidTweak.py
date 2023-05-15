# demo of csv file read
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime
from unite_pictures import unite_pictures_into_pdf
import os

def overall(old_s, new_s, filename, fig_files=None, plot_title=None, n_fig=None):
    if fig_files is None:
        fig_files = []
    plt.figure()
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title)
    plt.plot(old_s.time, old_s.Ib, color='green', label='Ib')
    plt.plot(new_s.time, new_s.Ib, color='orange', linestyle='--', label='Ib_new')
    plt.legend(loc=1)
    plt.subplot(222)
    plt.plot(old_s.time, old_s.sat, color='black', label='sat')
    plt.plot(new_s.time, new_s.sat, color='yellow', linestyle='--', label='sat_new')
    plt.plot(old_s.time, old_s.sel, color='red', label='sel')
    plt.plot(new_s.time, new_s.sel, color='blue', linestyle='--', label='sel_new')
    plt.plot(old_s.time, old_s.mod, color='blue', label='mod')
    plt.plot(new_s.time, new_s.mod, color='red', linestyle='--', label='sel_mod')
    plt.legend(loc=1)
    plt.subplot(223)
    plt.plot(old_s.time, old_s.Vb, color='green', label='Vb')
    plt.plot(new_s.time, new_s.Vb, color='orange', linestyle='--', label='Vb_new')
    plt.legend(loc=1)
    plt.subplot(224)
    plt.plot(old_s.time, old_s.Voc, color='green', label='Voc')
    plt.plot(new_s.time, new_s.Voc, color='orange', linestyle='--', label='Voc_new')
    plt.plot(old_s.time, old_s.Vsat, color='blue', label='Vsat')
    plt.plot(new_s.time, new_s.Vsat, color='red', linestyle='--', label='Vsat_new')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.subplot(221)
    plt.title(plot_title)
    plt.plot(old_s.time, old_s.Vdyn, color='green', label='Vdyn')
    plt.plot(new_s.time, new_s.Vdyn, color='orange', linestyle='--', label='Vdyn_new')
    plt.legend(loc=1)
    plt.subplot(222)
    plt.plot(old_s.time, old_s.Voc, color='green', label='Voc')
    plt.plot(new_s.time, new_s.Voc, color='orange', linestyle='--', label='Voc_new')
    plt.legend(loc=1)
    plt.subplot(223)
    plt.plot(old_s.time, old_s.Voc, color='green', label='Voc')
    plt.plot(new_s.time, new_s.Voc, color='orange', linestyle='--', label='Voc_new')
    plt.plot(old_s.time, old_s.Voc_ekf, color='blue', label='Voc_ekf')
    plt.plot(new_s.time, new_s.Voc_ekf, color='red', linestyle='--', label='Voc_ekf_new')
    plt.legend(loc=1)
    plt.subplot(224)
    plt.plot(old_s.time, old_s.y_ekf, color='green', label='y_ekf')
    plt.plot(new_s.time, new_s.y_ekf, color='orange', linestyle='--', label='y_ekf_new')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files


class Saved:
    def __init__(self, data=None):
        if data is None:
            self.time = []
            self.Ib = []  # Bank current, A
            self.Vb = []  # Bank voltage, V
            self.sat = []  # Indication that battery is saturated, T=saturated
            self.sel = []  # Current source selection, 0=amp, 1=no amp
            self.mod = []  # Configuration control code, 0=all hardware, 7=all simulated, +8 tweak test
            self.Tb = []  # Battery bank temperature, deg C
            self.Vsat = []  # Monitor Bank saturation threshold at temperature, deg C
            self.Vdyn = []  # Monitor Bank current induced back emf, V
            self.Voc = []  # Monitor Static bank open circuit voltage, V
            self.Voc_ekf = []  # Monitor bank solved static open circuit voltage, V
            self.y_ekf = []  # Monitor single battery solver error, V
            self.soc_m = []  # Simulated state of charge, fraction
            self.soc_ekf = []  # Solved state of charge, fraction
            self.soc = []  # Coulomb Counter fraction of saturation charge (q_capacity_) availabel (0-1)
            self.soc_wt = []  # Weighted selection of ekf state of charge and Coulomb Counter (0-1)
        else:
            self.time = data.cTime
            self.Ib = data.Ib
            self.Vb = data.Vb
            self.sat = data.sat
            self.sel = data.sel
            self.mod = data.mod
            self.Tb = data.Tb
            self.Vsat = data.Vsat
            self.Vdyn = data.Vdyn
            self.Voc = data.Voc
            self.Voc_ekf = data.Voc_ekf
            self.y_ekf = data.y_ekf
            self.soc_m = data.soc_m
            self.soc_ekf = data.soc_ekf
            self.soc = data.soc
            self.soc_wt = data.soc_wt
            # Find first non-zero Ib and use to adjust time
            # Ignore initial run of non-zero Ib because resetting from previous run
            zero_start = np.where(self.Ib == 0.0)[0][0]
            zero_end = zero_start
            while self.Ib[zero_end] == 0.0:  # stop at first non-zero
                zero_end += 1
            time_ref = self.time[zero_end]
            print("time_ref=", time_ref)
            self.time -= time_ref;


if __name__ == '__main__':
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt

    def main():
        # Load data
        # make csv file by hand from .ods by opening in openOffice and save as csv
        # data_file_old = './rapidTweakRegressionTest20220529_old.csv'
        # data_file_new = './rapidTweakRegressionTest20220529_new.csv'
        data_file_old = './slowCycleRegressionTest20220529_old.csv'
        data_file_new = './slowCycleRegressionTest20220529_new.csv'
        cols = (
        'unit', 'cTime', 'T', 'sat', 'sel', 'mod', 'Tb', 'Vb', 'Ib', 'Vsat', 'Vdyn', 'Voc', 'Voc_ekf', 'y_ekf', 'soc_m',
        'soc_ekf', 'soc', 'soc_wt')
        data_old = np.genfromtxt(data_file_old, delimiter=',', names=True, usecols=cols, dtype=None, encoding=None).view(
            np.recarray)
        data_new = np.genfromtxt(data_file_new, delimiter=',', names=True, usecols=cols, dtype=None, encoding=None).view(
            np.recarray)
        saved_old = Saved(data_old)
        saved_new = Saved(data_new)

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        n_fig, fig_files = overall(saved_old, saved_new, filename, fig_files, plot_title=plot_title, n_fig=n_fig)

        # Copies
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='figures')

        # Clean up after itself.   Other fig files already in root will get plotted by unite_pictures_into_pdf
        # Cleanup other figures in root folder by hand
        for fig_file in fig_files:
            try:
                os.remove(fig_file)
            except OSError:
                pass
        plt.show()


    main()
