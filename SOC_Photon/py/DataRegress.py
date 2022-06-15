# demo of csv file read
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime
from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
import os
from DataOverModel import overall, write_clean_file


class SavedData:
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
            self.Voc_dyn = []  # Bank VOC estimated from Vb and RC model, V
            self.Voc_ekf = []  # Monitor bank solved static open circuit voltage, V
            self.y_ekf = []  # Monitor single battery solver error, V
            self.soc_m = []  # Simulated state of charge, fraction
            self.soc_ekf = []  # Solved state of charge, fraction
            self.soc = []  # Coulomb Counter fraction of saturation charge (q_capacity_) availabel (0-1)
            self.soc_wt = []  # Weighted selection of ekf state of charge and Coulomb Counter (0-1)
            self.dv_hys = []  # Drop across hysteresis, V
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
            self.Voc_dyn = self.Vb - self.Vdyn
            self.dv_hys = self.Voc_dyn - self.Voc
            self.Voc_ekf = data.Voc_ekf
            self.y_ekf = data.y_ekf
            self.soc_m = data.soc_m
            self.soc_ekf = data.soc_ekf
            self.soc = data.soc
            self.soc_wt = data.soc_wt
            self.mod_data = data.mod
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
        # make txt file streaming from CoolTerm and v4 in logic, run Xp10 or 11
        # data_file = '../../../dataReduction/data_proto.csv'
        data_file_old_txt = '../dataReduction/rapidTweakRegressionTest20220613.txt'
        data_file_new_txt = '../dataReduction/rapidTweakRegressionTest20220613_new.txt'
        title_key = "unit,"     # Find one instance of title
        unit_key = 'pro_2022'  # Used to filter out actual data
        # Clean .txt file and load
        data_file_old_csv = write_clean_file(data_file_old_txt, '', title_key, unit_key)
        data_file_new_csv = write_clean_file(data_file_new_txt, '', title_key, unit_key)

        cols = (
        'unit', 'cTime', 'dt', 'sat', 'sel', 'mod', 'Tb', 'Vb', 'Ib', 'Vsat', 'Vdyn', 'Voc', 'Voc_ekf', 'y_ekf', 'soc_m',
        'soc_ekf', 'soc', 'soc_wt')
        data_old = np.genfromtxt(data_file_old_csv, delimiter=',', names=True, usecols=cols, dtype=None,
                                 encoding=None).view(np.recarray)
        data_new = np.genfromtxt(data_file_new_csv, delimiter=',', names=True, usecols=cols, dtype=None,
                                 encoding=None).view(np.recarray)
        saved_old = SavedData(data_old)
        saved_new = SavedData(data_new)

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        n_fig, fig_files = overall(saved_old, saved_new, filename, fig_files, plot_title=plot_title, n_fig=n_fig)

        # Copies
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='../dataReduction/figures')
        cleanup_fig_files(fig_files)

        plt.show()


    main()
