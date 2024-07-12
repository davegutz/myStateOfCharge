# PlotSimS - general purpose plotting, EKF related
# Copyright (C) 2023 Dave Gutz
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

"""Define a general purpose battery model including Randles' model and SoC-VOV model as well as Kalman filtering
support for simplified Mathworks' tracker. See Huria, Ceraolo, Gazzarri, & Jackey, 2013 Simplified Extended Kalman
Filter Observer for SOC Estimation of Commercial Power-Oriented LFP Lithium Battery Cells.
Dependencies:
    - numpy      (everything)
    - matplotlib (plots)
    - reportlab  (figures, pdf)
"""
import numpy as np
import matplotlib.pyplot as plt
from DataOverModel import plq
import sys
if sys.platform == 'darwin':
    import matplotlib
    matplotlib.use('tkagg')
plt.rcParams.update({'figure.max_open_warning': 0})


def ekf_plot(mo, mv, so, sv, smv, filename, fig_files=None, plot_title=None, fig_list=None,
             ref_str='_ref', test_str='_test'):
    if so and smv:
        if mo.Fx is not None:  # ekf
            fig_list.append(plt.figure())  # EKF  1
            plt.subplot(331)
            plt.title(plot_title + ' EKF 1')
            plt.plot(mo.time, mo.u, color='blue', linestyle='-', label='u' + ref_str)
            plt.plot(mv.time, mv.u_ekf, color='red', linestyle='--', label='u' + test_str)
            plt.legend(loc=1)
            plt.subplot(332)
            plt.plot(mo.time, mo.z, color='blue', linestyle='-', label='z' + ref_str)
            plt.plot(mv.time, mv.z_ekf, color='red', linestyle='--', label='z' + test_str)
            plt.legend(loc=1)
            plt.subplot(333)
            plt.plot(smv.time, smv.reset_s, color='green', linestyle='--', label='reset_s' + test_str)
            plt.plot(so.time, so.sat_s, color='blue', linestyle='-.', label='sat_s' + ref_str)
            plt.plot(smv.time, smv.sat_s, color='red', linestyle=':', label='sat_s' + test_str)
            plt.legend(loc=1)
            plt.subplot(334)
            plt.plot(mo.time, mo.Fx, color='blue', linestyle='-', label='Fx' + ref_str)
            plt.plot(mv.time, mv.Fx, color='red', linestyle='--', label='Fx' + test_str)
            plt.legend(loc=1)
            plt.subplot(335)
            plt.plot(mo.time, mo.Bu, color='blue', linestyle='-', label='Bu' + ref_str)
            plt.plot(mv.time, mv.Bu, color='red', linestyle='--', label='Bu' + test_str)
            plt.legend(loc=1)
            plt.subplot(336)
            plt.plot(mo.time, mo.Q, color='blue', linestyle='-', label='Q' + ref_str)
            plt.plot(mv.time, mv.Q, color='red', linestyle='--', label='Q' + test_str)
            plt.legend(loc=1)
            plt.subplot(337)
            plt.plot(mo.time, mo.R, color='blue', linestyle='-', label='R' + ref_str)
            plt.plot(mv.time, mv.R, color='red', linestyle='--', label='R' + test_str)
            plt.legend(loc=1)
            plt.subplot(338)
            plt.plot(mo.time, mo.P, color='blue', linestyle='-', label='P' + ref_str)
            plt.plot(mv.time, mv.P, color='red', linestyle='--', label='P' + test_str)
            plt.legend(loc=1)
            plt.subplot(339)
            plt.plot(mo.time, mo.S, color='blue', linestyle='-', label='S' + ref_str)
            plt.plot(mv.time, mv.S, color='red', linestyle='--', label='S' + test_str)
            plt.legend(loc=1)
            fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
            fig_files.append(fig_file_name)
            plt.savefig(fig_file_name, format="png")

            fig_list.append(plt.figure())  # EKF  2
            plt.subplot(331)
            plt.title(plot_title + ' EKF 2')
            plt.plot(mo.time, mo.K, color='blue', linestyle='-', label='K' + ref_str)
            plt.plot(mv.time, mv.K, color='red', linestyle='--', label='K' + test_str)
            plt.legend(loc=1)
            plt.subplot(332)
            plt.plot(mo.time, mo.x, color='blue', linestyle='-', label='x' + ref_str)
            plt.plot(mv.time, mv.x_ekf, color='red', linestyle='--', label='x' + test_str)
            plt.plot(mo.time, mo.soc_ekf, color='cyan', linestyle='-.', label='x=soc_ekf' + ref_str)
            plt.plot(mv.time, mv.soc_ekf, color='orange', linestyle=':', label='x=soc_ekf' + test_str)
            plt.legend(loc=1)
            plt.subplot(333)
            plt.plot(mo.time, mo.y, color='blue', linestyle='-', label='y' + ref_str)
            plt.plot(mv.time, mv.y_ekf, color='red', linestyle='--', label='y' + test_str)
            plt.legend(loc=1)
            plt.subplot(334)
            plt.plot(mo.time, mo.x_prior, color='blue', linestyle='-', label='x_prior' + ref_str)
            plt.plot(mv.time, mv.x_prior, color='red', linestyle='--', label='x_prior' + test_str)
            plt.legend(loc=1)
            plt.subplot(335)
            plt.plot(mo.time, mo.P_prior, color='blue', linestyle='-', label='P_prior' + ref_str)
            plt.plot(mv.time, mv.P_prior, color='red', linestyle='--', label='P_prior' + test_str)
            plt.legend(loc=1)
            plt.subplot(336)
            plt.plot(mo.time, mo.x_post, color='blue', linestyle='-', label='x_post' + ref_str)
            plt.plot(mv.time, mv.x_post, color='red', linestyle='--', label='x_post' + test_str)
            plt.legend(loc=1)
            plt.subplot(337)
            plt.plot(mo.time, mo.P_post, color='blue', linestyle='-', label='P_post' + ref_str)
            plt.plot(mv.time, mv.P_post, color='red', linestyle='--', label='P_post' + test_str)
            plt.legend(loc=1)
            plt.subplot(338)
            plt.plot(mo.time, mo.hx, color='blue', linestyle='-', label='hx' + ref_str)
            plt.plot(mv.time, mv.hx, color='red', linestyle='--', label='hx' + test_str)
            plt.legend(loc=1)
            plt.subplot(339)
            plt.plot(mo.time, mo.H, color='blue', linestyle='-', label='H' + ref_str)
            plt.plot(mv.time, mv.H, color='red', linestyle='--', label='H' + test_str)
            plt.legend(loc=1)
            fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
            fig_files.append(fig_file_name)
            plt.savefig(fig_file_name, format="png")

            fig_list.append(plt.figure())  # EKF3
            plt.subplot(221)
            plt.title(plot_title + ' EKF 3')
            plt.plot(mo.time, mo.ib, color='red', linestyle='-', label='ib' + ref_str)
            plt.plot(mv.time, mv.ib, color='black', linestyle='--', label='ib' + test_str)
            plt.plot(mo.time, mo.u, color='cyan', linestyle='-.', label='u' + ref_str)
            plt.plot(mv.time, mv.u_ekf, color='orange', linestyle=':', label='u' + test_str)
            plt.legend(loc=1)
            plt.subplot(222)
            plt.plot(mo.time, mo.vb, color='red', linestyle='-', label='vb' + ref_str)
            plt.plot(mv.time, mv.vb, color='black', linestyle='--', label='vb' + test_str)
            plt.plot(mo.time, mo.voc_stat, color='cyan', linestyle='-.', label='z=voc_stat' + ref_str)
            plt.plot(mv.time, mv.voc_stat, color='orange', linestyle=':', label='z=voc_stat' + test_str)
            plt.legend(loc=1)
            plt.subplot(223)
            plt.plot(mo.time, mo.soc, color='red', linestyle='-', label='soc' + ref_str)
            plt.plot(mv.time, mv.soc, color='black', linestyle='--', label='soc' + test_str)
            plt.plot(mo.time, mo.soc_ekf, color='cyan', linestyle='-.', label='x=soc_ekf' + ref_str)
            plt.plot(mv.time, mv.soc_ekf, color='orange', linestyle=':', label='x=soc_ekf' + test_str)
            plt.legend(loc=1)
            plt.subplot(224)
            plt.plot(mo.time, mo.voc_stat, color='red', linestyle='-', label='z=voc_stat' + ref_str)
            plt.plot(mv.time, mv.voc_stat, color='black', linestyle='--', label='z=voc_stat' + test_str)
            plt.plot(mo.time, mo.voc_ekf, color='cyan', linestyle='-.', label='voc_ekf(soc) = Hx' + ref_str)
            plt.plot(mv.time, mv.voc_ekf, color='orange', linestyle=':', label='voc_ekf(soc) = Hx' + test_str)
            plt.legend(loc=1)

    if mo.voc_soc is not None:
        fig_list.append(plt.figure())  # EKF  4
        plt.subplot(111)
        plt.title(plot_title + ' EKF 4')
        plt.plot(mo.soc, mo.voc_stat, color='red', linestyle='-', label='voc_stat' + ref_str)
        plt.plot(mo.soc, mo.voc_soc, color='black', linestyle=':', label='voc_soc' + ref_str)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        fig_list.append(plt.figure())  # Hyst 1
        plt.subplot(331)
        plt.title(plot_title + ' Hyst 1')
        # plt.plot(mo.time, mo.dv_hys_required, linestyle='-', color='black', label='dv_hys_required'+ref_str)
        if mo.e_wrap is not None:
            plt.plot(mo.time, -mo.e_wrap, linestyle='-', color='red', label='-e_wrap' + ref_str)
        plt.plot(mv.time, (np.array(mv.voc_stat) - np.array(mv.voc_soc)), linestyle='--', color='blue',
                 label='-e_wrap' + test_str)
        plt.plot(mo.time, mo.dv_hys, linestyle='-.', color='orange', label='dv_hys' + ref_str)
        plt.plot(mv.time, mv.dv_hys, marker='.', markersize='1', markevery=48, linestyle='None', color='magenta',
                 label='dv_hys' + test_str)
        if so is not None:
            plt.plot(so.time, so.dv_hys_s, linestyle=':', color='cyan', label='dv_hys_s' + ref_str)
        plq(plt, sv, 'time', svm, 'dv_hys', marker='.', markersize='1', markevery=64, linestyle='None', color='black',
                     label='dv_hys_s' + test_str)
        plq(plt, sv, 'time', sv, 'dv_hys_s', marker='.', markersize='1', markevery=64, linestyle='None', color='black',
                     label='dv_hys_s' + test_str)
        plt.xlabel('sec')
        plt.legend(loc=4)
        plt.subplot(332)
        # plt.plot(mo.soc, mo.dv_hys_required, linestyle='-', color='black', label='dv_hys_required'+ref_str)
        if mo.e_wrap is not None:
            plt.plot(mo.soc, -mo.e_wrap, linestyle='--', color='red', label='-e_wrap' + ref_str)
        plt.plot(mo.soc, mo.dv_hys, linestyle='-.', color='orange', label='dv_hys' + ref_str)
        plt.plot(mv.soc, mv.dv_hys, marker='.', markersize='1', markevery=4, linestyle='None', color='magenta',
                 label='dv_hys' + test_str)
        if so is not None:
            plt.plot(so.soc_s, so.dv_hys_s, linestyle=':', color='cyan', label='dv_hys_s' + ref_str)
        plq(plt, sv, 'soc', sv, 'dv_hys', marker='.', markersize='1', markevery=5, linestyle='None', color='black',
                     label='dv_hys_s' + test_str)
        plq(plt, sv, 'soc_s', sv, 'dv_hys_s', marker='.', markersize='1', markevery=5, linestyle='None', color='black',
                     label='dv_hys_s' + test_str)
        plt.xlabel('soc')
        plt.legend(loc=4)
        plt.subplot(333)
        plt.plot(mo.time, mo.soc, linestyle='-', color='green', label='soc' + ref_str)
        if so is not None:
            plt.plot(so.time, so.soc_s, linestyle='--', color='blue', label='soc_s' + ref_str)
        plt.plot(mv.time, mv.soc, linestyle='-.', color='red', label='soc' + test_str)
        plq(plt, sv, 'time', sv, 'soc', linestyle=':', color='cyan', label='soc_s' + test_str)
        plq(plt, sv, 'time', sv, 'soc_s', linestyle=':', color='cyan', label='soc_s' + test_str)
        plt.xlabel('sec')
        plt.legend(loc=4)
        plt.subplot(334)
        if mo.ib_sel is not None:
            plt.plot(mo.time, mo.ib_sel, linestyle='-', color='black', label='ib_sel' + ref_str)
        plt.plot(mo.time, mo.ioc, linestyle='--', color='cyan', label='ioc' + ref_str)
        plt.xlabel('sec')
        plt.legend(loc=4)
        plt.subplot(335)
        if mo.ib_sel is not None:
            plt.plot(mo.soc, mo.ib_sel, linestyle='-', color='black', label='ib_sel' + ref_str)
        plt.plot(mo.soc, mo.ioc, linestyle='--', color='cyan', label='ioc' + ref_str)
        plt.xlabel('soc')
        plt.legend(loc=4)
        plt.subplot(336)
        if so is not None:
            plt.plot(so.time, so.vb_s, color='black', linestyle='-', label='vb_s' + ref_str)
        plt.plot(mo.time, mo.vb, color='orange', linestyle='--', label='vb' + ref_str)
        if so is not None:
            plt.plot(so.time, so.voc_stat_s, color='magenta', linestyle='-', label='voc_stat_s' + ref_str)
        plt.plot(mo.time, mo.voc_stat, color='pink', linestyle='--', label='voc_stat' + ref_str)
        plt.plot(mo.time, mo.voc_soc, marker='.', markersize='1', markevery=32, linestyle='None', color='black',
                 label='voc_soc' + ref_str)
        plt.legend(loc=1)
        plt.subplot(337)
        plt.plot(mo.time, mo.vb, linestyle='-', color='green', label='vb' + ref_str)
        plt.plot(mo.time, mo.voc, linestyle='--', color='red', label='voc' + ref_str)
        plt.plot(mo.time, mo.voc_stat, linestyle='-.', color='pink', label='voc_stat' + ref_str)
        plt.plot(mo.time, mo.voc_soc, marker='.', markersize='1', markevery=32, linestyle='None', color='black',
                 label='voc_soc' + ref_str)
        plt.xlabel('sec')
        plt.legend(loc=4)
        plt.subplot(338)
        plt.plot(mo.soc, mo.vb, linestyle='-', color='green', label='vb' + ref_str)
        plt.plot(mo.soc, mo.voc, linestyle='--', color='red', label='voc' + ref_str)
        plt.plot(mo.soc, mo.voc_stat, linestyle='-.', color='blue', label='voc_stat' + ref_str)
        plt.plot(mo.soc, mo.voc_soc, linestyle=':', color='black', label='voc_soc' + ref_str)
        plt.xlabel('soc')
        plt.legend(loc=4)
        plt.subplot(339)
        plt.plot(mo.time, mo.vb, color='green', linestyle='-', label='vb' + ref_str)
        plt.plot(mv.time, mv.vb, color='orange', linestyle='--', label='vb' + test_str)
        plt.plot(mo.time, mo.voc, color='blue', linestyle='-.', label='voc' + ref_str)
        plt.plot(mv.time, mv.voc, marker='+', markersize='3', markevery=4, linestyle='None', color='black',
                 label='voc' + test_str)
        plt.plot(mo.time, mo.voc_stat, color='magenta', linestyle='-', label='voc_stat' + ref_str)
        plt.plot(mv.time, mv.voc_stat, color='pink', linestyle='--', label='voc_stat' + test_str)
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(len(fig_list)) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")
    return fig_list, fig_files
