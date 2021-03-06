# demo of csv file read
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime
from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
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

    plt.figure()
    n_fig += 1
    plt.subplot(131)
    plt.title(plot_title)
    plt.plot(old_s.time, old_s.soc, color='orange', label='soc')
    plt.plot(new_s.time, new_s.soc, color='green', linestyle='--', label='soc_new')
    plt.legend(loc=1)
    plt.subplot(132)
    plt.plot(old_s.time, old_s.Vb, color='orange', label='Vb')
    plt.plot(new_s.time, new_s.Vb, color='green', linestyle='--', label='Vb_new')
    plt.legend(loc=1)
    plt.subplot(133)
    plt.plot(old_s.soc, old_s.Vb, color='orange', label='Vb')
    plt.plot(new_s.soc, new_s.Vb, color='green', linestyle='--', label='Vb_new')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files


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


def write_clean_file(txt_file, title_str, unit_str):
    csv_file = txt_file.replace('.txt', '.csv', 1)
    default_header_str = "unit,               hm,                  cTime,        T,       sat,sel,mod,  Tb,  Vb,  Ib,        Vsat,Vdyn,Voc,Voc_ekf,     y_ekf,    soc_m,soc_ekf,soc,soc_wt,"
    # Header
    have_header_str = None
    with open(txt_file, "r") as input_file:
        with open(csv_file, "w") as output:
            for line in input_file:
                if line.__contains__(title_str):
                    if have_header_str is None:
                        have_header_str = True  # write one title only
                        output.write(line)
    if have_header_str is None:
        output.write(default_header_str)
        print("I:  using default data header")
    # Date
    with open(txt_file, "r") as input_file:
        with open(csv_file, "a") as output:
            for line in input_file:
                if line.__contains__(unit_str):
                    output.write(line)
    print("csv_file=", csv_file)
    return csv_file

if __name__ == '__main__':
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt

    def main():
        # Load data
        # make txt file streaming from CoolTerm and v4 in logic, run Xp10 or 11
        # data_file = '../../../dataReduction/data_proto.csv'
        data_file_old_txt = '../../../dataReduction/rapidTweakRegressionTest20220613.txt'
        data_file_new_txt = '../../../dataReduction/rapidTweakRegressionTest20220613_new.txt'
        title_str = "unit,"     # Find one instance of title
        unit_str = 'pro_2022'  # Used to filter out actual data
        # Clean .txt file and load
        data_file_old_csv = write_clean_file(data_file_old_txt, title_str, unit_str)
        data_file_new_csv = write_clean_file(data_file_new_txt, title_str, unit_str)

        cols = (
        'unit', 'cTime', 'T', 'sat', 'sel', 'mod', 'Tb', 'Vb', 'Ib', 'Vsat', 'Vdyn', 'Voc', 'Voc_ekf', 'y_ekf', 'soc_m',
        'soc_ekf', 'soc', 'soc_wt')
        data_old = np.genfromtxt(data_file_old_csv, delimiter=',', names=True, usecols=cols, dtype=None, encoding=None).view(np.recarray)
        data_new = np.genfromtxt(data_file_new_csv, delimiter=',', names=True, usecols=cols, dtype=None, encoding=None).view(np.recarray)
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
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo='../../../dataReduction/figures')
        cleanup_fig_files(fig_files)

        plt.show()


    main()
