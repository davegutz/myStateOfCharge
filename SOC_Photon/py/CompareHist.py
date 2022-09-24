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

from pyDAGx import myTables

RATED_TEMP = 25.  # Temperature at RATED_BATT_CAP, deg C
BATT_DVOC_DT = 0.004  # 5/30/2022
bat_v_sat = 13.8


# Unix-like cat function
# e.g. > cat('out', ['in0', 'in1'], path_to_in='./')
def cat(out_file_name, in_file_names, in_path='./', out_path='./'):
    with open(out_path+'./'+out_file_name, 'w') as out_file:
        for in_file_name in in_file_names:
            with open(in_path+'/'+in_file_name) as in_file:
                for line in in_file:
                    if line.strip():
                        out_file.write(line)


class SavedHist:
    def __init__(self, data=None, schedule=None):
        if data is None:
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
            self.dscn_fa = None
            self.ib_diff_fa = None
            self.wv_fa = None
            self.wl_fa = None
            self.wh_fa = None
            self.ccd_fa = None
            self.ib_noa_fa = None
            self.ib_amp_fa = None
            self.vb_fa = None
            self.tb_fa = None
            self.voc_soc = None
        else:
            # Load raw data.  Assume soc is accurate (GIGO)
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
            # Convert time to decimal days
            min_time = np.min(self.time)
            self.time = (self.time - min_time)/3600./24.
            falw = np.array(self.falw, dtype=np.uint16)
            self.dscn_fa = np.bool8(np.array(falw) & 2**10)
            self.ib_diff_fa = np.bool8((np.array(falw) & 2**8) | (np.array(falw) & 2**9))
            self.wv_fa = np.bool8(np.array(falw) & 2**7)
            self.wl_fa = np.bool8(np.array(falw) & 2**6)
            self.wh_fa = np.bool8(np.array(falw) & 2**5)
            self.ccd_fa = np.bool8(np.array(falw) & 2**4)
            self.ib_noa_fa = np.bool8(np.array(falw) & 2**3)
            self.ib_amp_fa = np.bool8(np.array(falw) & 2**2)
            self.vb_fa = np.bool8(np.array(falw) & 2**1)
            self.tb_fa = np.bool8(np.array(falw) & 2**0)
            # Add the schedule for reference
            if schedule is not None:
                self.voc_soc = []
                self.Vsat = []
                for i in range(len(self.time)):
                    self.voc_soc.append(schedule.interp(self.soc[i], self.Tb[i]))
                    self.Vsat.append(bat_v_sat + (self.Tb[i] - RATED_TEMP) * BATT_DVOC_DT)


    def __str__(self):
        s = ""
        for i in range(len(self.date)):
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
    # Markers
    #
    # light symbols
    # '.' point
    # ',' pixel
    # '1' tri down
    # '2' tri up
    # '3' tri left
    # '4' tri right
    # '+' plus
    # 'x' x
    # '|' vline
    # '_' hline
    # 0 (TICKLEFT) tickleft
    # 1 (TICKRIGHT) tickright
    # 2 (TICKUP) tickup
    # 3 (TICKDOWN) tickdown
    #
    # bold filled symbols
    # 'o' circle
    # 'v' triangle down
    # '^' triangle up
    # '<' triangle left
    # '>' triangle right
    # '8' octagon
    # 's' square
    # 'p' pentagon
    # 'P' filled plus
    # '*' star
    # 'h' hexagon1
    # 'H' hexagon2
    # 4 (CARETLEFT) caretleft
    # 5 (CARETRIGHT) caretright
    # 6 (CARETUP) caretup
    # 7 (CARETDOWN) caretdown
    # 8 (CARETLEFTBASE) caretleft centered at base
    # 9 (CARETRIGHTBASE) caretright centered at base
    # 10 (CARETUPBASE) caretup centered at base
    # 11 (CARETDOWNBASE) caretdown centered at base

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

    plt.figure()  # 2
    n_fig += 1
    plt.subplot(221)
    plt.plot(hi.time, hi.Vsat, marker='^', markersize='3', linestyle='None', color='red', label='Vsat')
    plt.plot(hi.time, hi.Vb, marker='1', markersize='3', linestyle='None', color='black', label='Vb')
    plt.plot(hi.time, hi.Voc_dyn,marker='.', markersize='3', linestyle='None', color='orange', label='Voc_dyn')
    plt.plot(hi.time, hi.Voc_stat, marker='_', markersize='3', linestyle='None', color='green', label='Voc_stat')
    plt.plot(hi.time, hi.voc_soc, marker='2', markersize='3', linestyle='None', color='cyan', label='voc_soc')
    plt.legend(loc=1)
    plt.subplot(122)
    plt.plot(hi.time, hi.dscn_fa + 10, marker='o', markersize='3', linestyle='None', color='black', label='dscn_fa+10')
    plt.plot(hi.time, hi.ib_diff_fa + 8, marker='^', markersize='3', linestyle='None', color='blue', label='ib_diff_fa+8')
    plt.plot(hi.time, hi.wv_fa + 7, marker='s', markersize='3', linestyle='None', color='cyan', label='wrap_vb_fa+7')
    plt.plot(hi.time, hi.wl_fa + 6, marker='p', markersize='3', linestyle='None', color='orange', label='wrap_lo_fa+6')
    plt.plot(hi.time, hi.wh_fa + 5,marker='h', markersize='3', linestyle='None', color='green', label='wrap_hi_fa+5')
    plt.plot(hi.time, hi.ccd_fa + 4, marker='H', markersize='3', linestyle='None', color='blue', label='cc_diff_fa+4')
    plt.plot(hi.time, hi.ib_noa_fa + 3, marker='+', markersize='3', linestyle='None', color='red', label='ib_noa_fa+3')
    plt.plot(hi.time, hi.ib_amp_fa + 2, marker='_', markersize='3', linestyle='None', color='magenta', label='ib_amp_fa+2')
    plt.plot(hi.time, hi.vb_fa + 1, marker='1', markersize='3', linestyle='None', color='cyan', label='vb_fa+1')
    plt.plot(hi.time, hi.tb_fa, marker='2', markersize='3', linestyle='None', color='orange', label='tb_fa')
    plt.legend(loc=1)
    plt.subplot(223)
    plt.plot(hi.time, hi.Ib, marker='.', markersize='3', linestyle='None', color='red', label='Ib')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 3
    n_fig += 1
    plt.subplot(111)
    plt.plot(hi.soc, hi.Voc_stat, marker='3', markersize='3', linestyle='None', color='magenta', label='Voc_stat')
    plt.plot(hi.soc, hi.voc_soc, marker='_', markersize='2', linestyle='None', color='black', label='Schedule')
    plt.legend(loc=1)
    plt.title(plot_title)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    plt.figure()  # 4
    n_fig += 1
    plt.subplot(111)
    plt.plot(hi.soc, hi.Voc_stat, marker=0, markersize='3', linestyle='None', color='red', label='Voc_stat')
    plt.plot(hi.soc, hi.voc_soc, marker='_', markersize='2', linestyle='None', color='black', label='Schedule')
    plt.legend(loc=1)
    plt.title(plot_title)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")
    plt.ylim(10, 13.5)
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

        # Battleborn Bmon=0, Bsim=0
        t_x_soc0 = [-0.15, 0.00, 0.05, 0.10, 0.14, 0.17,  0.20,  0.25,  0.30,  0.40,  0.50,  0.60,  0.70,  0.80,  0.90,  0.99,  0.995, 1.00]
        t_y_t0 = [5.,  11.1,  20.,   30.,   40.]
        t_voc0 = [4.00, 4.00,  9.00,  11.80, 12.50, 12.60, 12.67, 12.76, 12.82, 12.93, 12.98, 13.03, 13.07, 13.11, 13.17, 13.22, 13.59, 14.45,
                  4.00, 4.00,  10.00, 12.30, 12.60, 12.65, 12.71, 12.80, 12.90, 12.96, 13.01, 13.06, 13.11, 13.17, 13.20, 13.23, 13.60, 14.46,
                  4.00, 12.22, 12.66, 12.74, 12.80, 12.85, 12.89, 12.95, 12.99, 13.05, 13.08, 13.13, 13.18, 13.21, 13.25, 13.27, 13.72, 14.50,
                  4.00, 4.00, 12.00, 12.65, 12.75, 12.80, 12.85, 12.95, 13.00, 13.08, 13.12, 13.16, 13.20, 13.24, 13.26, 13.27, 13.72, 14.50,
                  4.00, 4.00, 4.00,  4.00,  10.50, 11.93, 12.78, 12.83, 12.89, 12.97, 13.06, 13.10, 13.13, 13.16, 13.19, 13.20, 13.72, 14.50]
        x0 = np.array(t_x_soc0)
        y0 = np.array(t_y_t0)
        data_interp0 = np.array(t_voc0)
        lut_voc = myTables.TableInterp2D(x0, y0, data_interp0)

        # Save these

        # Regression suite
        # data_file_old_txt = 'install20220914.txt';
        input_files = ['hist 20220917d-1.txt']
        title_key = "hist"  # Find one instance of title
        unit_key = 'unit_h'  # happens to be what the long int of time starts with
        path_to_pdfs = '../dataReduction/figures'
        path_to_data = '../dataReduction'
        path_to_temp = '../dataReduction/temp'
        # cat files
        data_file_old_txt = 'compare_hist_temp.txt'
        cat(data_file_old_txt, input_files, in_path=path_to_data, out_path=path_to_temp)

        # Load mon v4 (old)
        data_file_clean = write_clean_file(data_file_old_txt, type_='_hist', title_key=title_key, unit_key=unit_key,
                                           skip=skip, path_to_data=path_to_temp, path_to_temp=path_to_temp,
                                           comment_str='---')
        if data_file_clean:
            cols = ('date', 'time', 'Tb', 'Vb', 'Ib', 'soc', 'soc_ekf', 'Voc_dyn', 'Voc_stat', 'tweak_sclr_amp',
                    'tweak_sclr_noa', 'falw')
            hist_old_raw = np.genfromtxt(data_file_clean, delimiter=',', names=True, usecols=cols,
                                         dtype=None, encoding=None).view(np.recarray)
            # Sort unique
            hist_old_unique = np.unique(hist_old_raw)
            hist_old = SavedHist(data=hist_old_unique, schedule=lut_voc)
        else:
            print("data from", data_file_old_txt, "empty after loading")
            exit(1)
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
