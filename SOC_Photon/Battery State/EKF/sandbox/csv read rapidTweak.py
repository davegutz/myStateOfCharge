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
    plt.subplot(211)
    plt.title(plot_title)
    plt.plot(old_s.time, old_s.Ib, color='green', label='Ib')
    plt.plot(new_s.time, new_s.Ib, color='orange', linestyle='--', label='Ib_new')
    plt.legend(loc=1)
    plt.subplot(212)
    plt.plot(old_s.time, old_s.Vb, color='green', label='Vb')
    plt.plot(new_s.time, new_s.Vb, color='orange', linestyle='--', label='Vb_new')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()
    n_fig += 1
    plt.subplot(211)
    plt.title(plot_title)
    plt.plot(old_s.time, old_s.Ib, color='green', label='Ib')
    plt.plot(new_s.time, new_s.Ib, color='orange', linestyle='--', label='Ib_new')
    plt.legend(loc=1)
    plt.subplot(212)
    plt.plot(old_s.time, old_s.Vb, color='green', label='Vb')
    plt.plot(new_s.time, new_s.Vb, color='orange', linestyle='--', label='Vb_new')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files


class Saved:
    def __init__(self):
        self.time = []
        self.Ib = []
        self.Vb = []


if __name__ == '__main__':
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt

    def main():
        # make csv file by hand from .ods by opening in openOffice and save as csv
        # data_file = '../../../dataReduction/data_proto.csv'
        data_file_old = '../../../dataReduction/rapidTweakRegressionTest20220529_old.csv'
        data_file_new = '../../../dataReduction/rapidTweakRegressionTest20220529_new.csv'
        cols = (
        'unit', 'cTime', 'T', 'sat', 'mod', 'Tb', 'Vb', 'Ib', 'Vsat', 'Vdyn', 'Voc', 'Voc_ekf', 'y_ekf', 'soc_m',
        'soc_ekf', 'soc', 'soc_wt')
        data_old = np.genfromtxt(data_file_old, delimiter=',', names=True, usecols=cols, dtype=None, encoding=None).view(
            np.recarray)
        data_new = np.genfromtxt(data_file_new, delimiter=',', names=True, usecols=cols, dtype=None, encoding=None).view(
            np.recarray)

        saved_old = Saved()
        saved_old.time = data_old.cTime
        saved_old.Ib = data_old.Ib
        saved_old.Vb = data_old.Vb
        # TODO:  find first non-zero Ib and use to adjust time
        time_ref = saved_old.time[0]
        saved_old.time -= time_ref;
        saved_new = Saved()
        saved_new.time = data_new.cTime
        saved_new.Ib = data_new.Ib
        saved_new.Vb = data_new.Vb
        time_ref = saved_new.time[0]
        saved_new.time -= time_ref;

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        n_fig, fig_files = overall(saved_old, saved_new, filename, fig_files,
                                   plot_title=plot_title, n_fig=n_fig)

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
