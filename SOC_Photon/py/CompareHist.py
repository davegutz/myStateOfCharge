# MonSim:  Monitor and Simulator replication of Particle Photon Application
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
# See http://www.fsf.org/licensing/licenses/lgpl.txt for full license text.

""" Slice and dice the history dumps."""


class SavedHist:
    def __init__(self, data=None):
        if data is None:
            self.i = None
            self.date = None
            self.time = None
            self.Tb = None
            self.Vb = None
            self.Ib = None
            self.soc = None
            self.soc_ekf = None
            self.Voc_dyn = None
            self.Voc_stat = None
            self.tweak_sclr_amp = None
            self.tweak_sclr_noa = None
            self.falw = None
        else:
            self.i = np.array(data.i)
            self.date = np.array(data.date)
            self.time = np.array(data.time)
            self.Tb = np.array(data.Tb)
            self.Vb = np.array(data.Vb)
            self.Ib = np.array(data.Ib)
            self.soc = np.array(data.soc)
            self.soc_ekf = np.array(data.soc_ekf)
            self.Voc_dyn = np.array(data.Voc_dyn)
            self.Voc_stat = np.array(data.Voc_stat)
            self.tweak_sclr_amp = np.array(data.tweak_sclr_amp)
            self.tweak_sclr_noa = np.array(data.tweak_sclr_noa)
            self.falw = np.array(data.falw)

    def __str__(self):
        s = ""
        for i in range(len(self.i)):
            s += "{},".format(self.date[i])
            s += "{},".format(self.time[i])
            s += "{:5.2f},".format(self.Tb[i])
            s += "{:7.3f},".format(self.Vb[i])
            s += "{:8.3f},".format(self.Ib[i])
            s += "{:5.3f},".format(self.soc[i])
            s += "{:5.3f},".format(self.soc_ekf[i])
            s += "{:5.3f},".format(self.Voc_dyn[i])
            s += "{:5.3f},".format(self.Voc_stat[i])
            s += "{:7.3f},".format(self.tweak_sclr_amp[i])
            s += "{:7.3f},".format(self.tweak_sclr_noa[i])
            s += "{:6.0f},\n".format(self.falw[i])
        return s


def overall_hist(hi, filename, fig_files=None, plot_title=None, n_fig=None):
    if fig_files is None:
        fig_files = []

    plt.figure()  # 1
    n_fig += 1
    plt.subplot(321)
    plt.title(plot_title)
    plt.plot(hi.time, hi.soc, marker='.', markersize='3', linestyle='None', color='black', label='soc')
    plt.plot(hi.time, hi.soc_ekf, marker='.', markersize='3', linestyle='None', color='green', label='soc_ekf')
    plt.legend(loc=1)
    plt.subplot(322)
    plt.plot(hi.time, hi.Tb, marker='.', markersize='3', linestyle='None', color='black', label='Tb')
    plt.plot(hi.time, hi.Vb, marker='.', markersize='3', linestyle='None', color='red', label='Vb')
    plt.plot(hi.time, hi.Voc_dyn, marker='.', markersize='3', linestyle='None', color='blue', label='Voc_dyn')
    plt.plot(hi.time, hi.Voc_stat, marker='.', markersize='3', linestyle='None', color='green', label='Voc_stat')
    plt.legend(loc=1)
    plt.subplot(323)
    plt.plot(hi.time, hi.Ib, marker='+', markersize='3', linestyle='None', color='green', label='Ib')
    plt.legend(loc=1)
    plt.subplot(324)
    plt.plot(hi.time, hi.tweak_sclr_amp, marker='+', markersize='3', linestyle='None', color='orange', label='tweak_sclr_amp')
    plt.plot(hi.time, hi.tweak_sclr_noa, marker='^', markersize='3', linestyle='None', color='green', label='tweak_sclr_noa')
    plt.ylim(-6, 6)
    plt.legend(loc=1)
    plt.subplot(325)
    plt.plot(hi.time, hi.falw, marker='+', markersize='3', linestyle='None', color='magenta', label='falw')
    plt.legend(loc=1)
    plt.subplot(326)
    plt.plot(hi.soc, hi.soc_ekf, marker='+', markersize='3', linestyle='None', color='magenta', label='soc_ekf')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 1
    n_fig += 1
    plt.subplot(111)
    # plt.plot(hi.soc, hi.Vb, marker='.', markersize='3', linestyle='None', color='red', label='Vb')
    # plt.plot(hi.soc, hi.Voc_dyn, marker='.', markersize='3', linestyle='None', color='blue', label='Voc_dyn')
    plt.plot(hi.soc, hi.Voc_stat, marker='.', markersize='3', linestyle='None', color='green', label='Voc_stat')
    plt.legend(loc=1)
    plt.title(plot_title)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files


if __name__ == '__main__':
    import numpy as np
    import sys
    from MonSim import replicate, save_clean_file, save_clean_file_sim
    from DataOverModel import SavedData, SavedDataSim, write_clean_file, overall
    from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files, precleanup_fig_files
    import matplotlib.pyplot as plt
    plt.rcParams['axes.grid'] = True
    from datetime import datetime, timedelta

    def main():
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        date_ = datetime.now().strftime("%y%m%d")
        skip = 1

        # Save these

        # Regression suite
        data_file_old_txt = 'install20220914.txt';
        title_key = "Voc_dyn"  # Find one instance of title
        unit_key = ', 1663'  # happens to be what the long int of time starts with
        path_to_pdfs = '../dataReduction/figures'
        path_to_data = '../dataReduction'
        path_to_temp = '../dataReduction/temp'

        # Load mon v4 (old)
        data_file_clean = write_clean_file(data_file_old_txt, type_='_hist', title_key=title_key, unit_key=unit_key,
                                           skip=skip, path_to_data=path_to_data, path_to_temp=path_to_temp)
        if data_file_clean:
            cols = ('i', 'date', 'time', 'Tb', 'Vb', 'Ib', 'soc', 'soc_ekf', 'Voc_dyn', 'Voc_stat', 'tweak_sclr_amp',
                    'tweak_sclr_noa', 'falw')
            hist_old_raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols,  dtype=None,
                                        encoding=None).view(np.recarray)
            hist_old = SavedHist(data=hist_old_raw)
        else:
            hist_old = None
        print(hist_old)

        # Plots
        n_fig = 0
        fig_files = []
        data_root = data_file_clean.split('/')[-1].replace('.csv', '-')
        filename = data_root + sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time
        n_fig, fig_files = overall_hist(hist_old, filename, fig_files=fig_files, plot_title=plot_title, n_fig=n_fig)
        precleanup_fig_files(output_pdf_name=filename, path_to_pdfs=path_to_pdfs)
        unite_pictures_into_pdf(outputPdfName=filename+'_'+date_time+'.pdf', pathToSavePdfTo=path_to_pdfs)
        cleanup_fig_files(fig_files)

        plt.show()

    main()
