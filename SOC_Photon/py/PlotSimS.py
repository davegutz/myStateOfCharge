# PlotSimS - general purpose plotting, sim related
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
from datetime import datetime
from MonSim import replicate, save_clean_file, save_clean_file_sim
from Battery import overall_batt
# below suppresses runtime error display******************
# import os
# os.environ["KIVY_NO_CONSOLELOG"] = "1"
# from kivy.utils import platform  # failed experiment to run BLE data plotting realtime on android
# if platform != 'linux':
#     from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
from unite_pictures import unite_pictures_into_pdf, cleanup_fig_files
plt.rcParams.update({'figure.max_open_warning': 0})


def sim_s_plot(mo, mv, so, sv, smv, filename, fig_files=None, plot_title=None, n_fig=None, plot_init_in=False,
               old_str='_old', new_str='_new'):
    if so and smv:
        plt.figure()  # sim_s  1
        n_fig += 1
        plt.subplot(331)
        plt.title(plot_title + ' sim_s 1')
        plt.plot(mo.time, mo.ib_sel, color='blue',  linestyle='-', label='ib_sel=ib_in'+old_str)
        if hasattr(mv, 'ib_in'):
            plt.plot(mv.time, mv.ib_in, color='cyan', linestyle='--', label='ib_in'+new_str)
        plt.plot(so.time, so.ib_in_s, color='green', linestyle='-.', label='ib_in_s'+old_str)
        plt.plot(smv.time, smv.ib_in_s, color='orange', linestyle=':', label='ib_in_s'+new_str)
        plt.plot(mo.time, np.array(mo.ib_charge)-1, color='black',  linestyle='-', label='ib_charge'+old_str+'-1')
        plt.plot(mv.time, np.array(mv.ib_charge)-1, color='orange', linestyle='--', label='ib_charge'+new_str+'-1')
        plt.plot(so.time, np.array(so.ib_charge_s)-1, color='blue', linestyle='-.', label='ib_charge_s'+old_str+'-1')
        plt.plot(smv.time, np.array(smv.ib_charge_s)-1, color='red', linestyle=':', label='ib_charge_s'+new_str+'-1')
        plt.plot(so.time, np.array(so.ib_in_s)+1, color='green', linestyle='-.', label='ib_in_s'+old_str+'+1')
        plt.plot(smv.time, np.array(smv.ib_in_s)+1, color='orange', linestyle=':', label='ib_in_s'+new_str+'+1')
        plt.legend(loc=1)
        plt.subplot(332)
        plt.plot(so.time, so.soc_s, color='magenta', linestyle='-', label='soc_s'+old_str)
        plt.plot(smv.time, smv.soc_s, color='green', linestyle='--', label='soc_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(333)
        plt.plot(so.time, so.voc_stat_s, color='magenta', linestyle='-', label='voc_stat_s'+old_str)
        plt.plot(smv.time, smv.voc_stat_s, color='green', linestyle='--', label='voc_stat_s'+new_str)
        plt.plot(so.time, so.vsat_s, color='blue',  linestyle='-.', label='vsat_s'+old_str)
        plt.plot(smv.time, smv.vsat_s, color='cyan', linestyle=':', label='vsat_s'+new_str)
        plt.plot(so.time, so.vb_s, color='orange', linestyle='-.', label='vb_s'+old_str)
        plt.plot(smv.time, smv.vb_s, color='black', linestyle=':', label='vb_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(334)
        plt.plot(so.time, so.Tb_s, color='magenta', linestyle='-', label='Tb_s'+old_str)
        plt.plot(smv.time, smv.Tb_s, color='green', linestyle='--', label='Tb_s'+new_str)
        plt.plot(so.time, so.Tbl_s, color='blue', linestyle='-.', label='Tbl_s'+old_str)
        plt.plot(smv.time, smv.Tbl_s, color='cyan', linestyle=':', label='Tbl_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(335)
        plt.plot(so.time, so.dv_dyn_s, color='magenta', linestyle='-', label='dv_dyn_s'+old_str)
        plt.plot(smv.time, smv.dv_dyn_s, color='green', linestyle='--', label='dv_dyn_s'+new_str)
        plt.plot(mo.time, mo.dv_dyn, color='blue', linestyle='-.', label='dv_dyn'+old_str)
        plt.plot(mv.time, mv.dv_dyn, color='cyan', linestyle=':', label='dv_dyn'+new_str)
        plt.legend(loc=1)
        plt.subplot(336)
        plt.plot(mo.time, mo.ib_sel, color='blue', linestyle='-', label='ib_sel'+old_str)
        plt.plot(so.time, so.ib_s, color='red', linestyle='--', label='ib_s'+old_str)
        plt.plot(mo.time, mo.ioc, color='cyan', linestyle='-', label='ioc'+old_str)
        plt.plot(mv.time, mv.ioc, color='magenta', linestyle='--', label='ioc'+new_str)
        plt.plot(so.time, so.ioc_s, color='green', linestyle='-.', label='ioc_s'+old_str)
        if hasattr(sv, 'ioc'):
            plt.plot(sv.time, sv.ioc, color='black', linestyle=':', label='ioc_s'+new_str)
        elif hasattr(sv, 'ioc_s'):
            plt.plot(sv.time, sv.ioc_s, color='black', linestyle=':', label='ioc_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(337)
        plt.plot(so.time, so.dq_s, color='magenta', linestyle='-', label='dq_s'+old_str)
        plt.plot(smv.time, smv.dq_s, color='green', linestyle='--', label='dq_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(338)
        plt.plot(so.time, so.reset_s, color='magenta', linestyle='-', label='reset_s'+old_str)
        plt.plot(smv.time, smv.reset_s, color='green', linestyle='--', label='reset_s'+new_str)
        plt.plot(so.time, so.sat_s, color='blue', linestyle='-.', label='sat_s'+old_str)
        plt.plot(smv.time, smv.sat_s, color='red', linestyle=':', label='sat_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(339)
        plt.plot(mo.time, mo.chm, color='blue', linestyle='-', label='chem'+old_str)
        plt.plot(mv.time, mv.chm, color='red', linestyle='--', label='chm'+new_str)
        plt.plot(so.time, so.chm_s, color='green', linestyle='-.', label='chem_s'+old_str)
        plt.plot(smv.time, smv.chm_s, color='orange', linestyle=':', label='smv.chm_s'+new_str)
        if hasattr(sv, 'chm'):
            plt.plot(sv.time, np.array(sv.chm)+4, color='red', linestyle='-', label='sv.chm'+new_str+'+4')
        elif hasattr(sv, 'chm_s'):
            plt.plot(sv.time, np.array(sv.chm_s)+4, color='red', linestyle='-', label='sv.chm'+new_str+'+4')
        plt.plot(smv.time, np.array(smv.chm_s)+4, color='black', linestyle='--', label='smv.chm_s'+new_str+'+4')
        plt.legend(loc=1)
        fig_file_name = filename + '_' + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        plt.figure()  # sim_s  2
        n_fig += 1
        plt.subplot(331)
        plt.title(plot_title + ' sim_s 2')
        plt.plot(mo.time, mo.vb, color='red', linestyle='-', label='vb'+old_str)
        plt.plot(mo.time, mo.voc, color='black',  linestyle='--', label='voc'+old_str)
        plt.plot(mo.time, mo.voc_stat, color='blue', linestyle='-.', label='voc_stat'+old_str)
        if mo.vb_h is not None:
            plt.plot(mo.time, mo.voc_soc, color='orange', linestyle=':', label='voc_soc'+old_str)
            plt.plot(mo.time, mo.vb_h, color='cyan', linestyle=':', label='vb_hdwe'+old_str)
        plt.legend(loc=1)
        plt.subplot(332)
        plt.plot(so.time, so.vb_s, color='red', linestyle='-', label='vb_s'+old_str)
        plt.plot(so.time, so.voc_s, color='black',  linestyle='--', label='voc_s'+old_str)
        plt.plot(so.time, so.voc_stat_s, color='blue', linestyle='-.', label='voc_stat_s'+old_str)
        plt.legend(loc=1)
        plt.subplot(334)
        plt.plot(mv.time, mv.vb, color='red', linestyle='-', label='vb'+new_str)
        plt.plot(mv.time, mv.voc, color='black', linestyle='--', label='voc'+new_str)
        plt.plot(mv.time, mv.voc_stat, color='blue', linestyle='-.', label='voc_stat'+new_str)
        plt.legend(loc=1)
        plt.subplot(335)
        plt.plot(smv.time, smv.vb_s, color='red', linestyle='-', label='vb_s'+new_str)
        plt.plot(smv.time, smv.voc_s, color='black', linestyle='--', label='voc_s'+new_str)
        plt.plot(mv.time, mv.voc_soc, color='blue', linestyle='-.', label='voc_soc'+new_str)
        plt.plot(smv.time, smv.voc_stat_s, color='orange', linestyle=':', label='voc_stat_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(337)
        plt.plot(mo.time, mo.soc, color='red', linestyle='-', label='soc'+old_str)
        plt.plot(mo.time, mo.soc_ekf, color='black',  linestyle='--', label='soc_ekf'+old_str)
        plt.plot(mv.time, mv.soc, color='blue', linestyle='-.', label='soc'+new_str)
        plt.plot(mv.time, mv.soc_ekf, color='orange', linestyle=':', label='soc_ekf'+new_str)
        plt.legend(loc=1)
        plt.subplot(338)
        plt.plot(so.time, so.soc_s, color='orange', linestyle='-', label='soc_s'+old_str)
        plt.plot(smv.time, smv.soc_s, color='cyan', linestyle='--', label='soc_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(333)
        plt.plot(mo.time, mo.vb, color='red', linestyle='-', label='vb'+old_str)
        plt.plot(so.time, so.vb_s, color='black', linestyle='--',  label='vb_s'+old_str)
        plt.plot(mv.time, mv.vb, color='blue', linestyle='-.', label='vb'+new_str)
        plt.plot(smv.time, smv.vb_s, color='orange', linestyle=':', label='vb_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(336)
        plt.plot(mo.time, mo.voc, color='red', linestyle='-', label='voc'+old_str)
        plt.plot(mv.time, mv.voc, color='black', linestyle='--', label='voc'+new_str)
        plt.plot(so.time, so.voc_s, color='blue', linestyle='-.', label='voc_s'+old_str)
        plt.plot(smv.time, smv.voc_s, color='orange', linestyle=':', label='voc_s'+new_str)
        plt.legend(loc=1)
        plt.subplot(339)
        plt.plot(mo.time, mo.voc_stat, color='red', linestyle='-', label='voc_stat'+old_str)
        plt.plot(so.time, so.voc_stat_s, color='black',  linestyle='--', label='voc_stat_s'+old_str)
        plt.plot(mv.time, mv.voc_stat, color='blue', linestyle='-.', label='voc_stat'+new_str)
        plt.plot(smv.time, smv.voc_stat_s, color='orange', linestyle=':', label='voc_stat_s'+new_str)
        plt.legend(loc=1)

        plt.figure()  # sim_s  3
        n_fig += 1
        plt.subplot(321)
        plt.title(plot_title + ' sim_s 3')
        plt.plot(mo.time, mo.soc, color='blue', linestyle='-', label='soc'+old_str)
        plt.plot(mv.time, mv.soc, color='red', linestyle='--', label='soc'+new_str)
        plt.plot(so.time, so.soc_s, color='green', linestyle='-.', label='soc_s'+old_str)
        if hasattr(sv, 'soc'):
            plt.plot(sv.time, sv.soc, color='orange', linestyle=':', label='sv.soc_s'+new_str)
        elif hasattr(sv, 'soc_s'):
            plt.plot(sv.time, sv.soc_s, color='orange', linestyle=':', label='sv.soc_s'+new_str)
        plt.plot(mo.time, mo.soc_ekf, color='cyan', linestyle='-', label='soc_ekf'+old_str)
        plt.plot(mv.time, mv.soc_ekf, color='magenta', linestyle='--', label='soc_ekf'+new_str)
        if hasattr(sv, 'soc'):
            plt.plot(sv.time, np.array(sv.soc)-.2, color='orange', linestyle='-', label='sv.soc_s'+new_str+'-0.2')
        elif hasattr(sv, 'soc_s'):
            plt.plot(sv.time, np.array(sv.soc_s)-.2, color='orange', linestyle='-', label='sv.soc_s'+new_str+'-0.2')
        plt.plot(smv.time, np.array(smv.soc_s)-.2, color='black', linestyle='--', label='smv.soc_s'+new_str+'-0.2')
        plt.legend(loc=1)
        plt.subplot(322)
        if mo.vb_h is not None and max(mo.vb_h) > 1.:
            plt.plot(mo.soc, mo.vb_h, color='magenta', linestyle=':', label='vb_hdwe'+old_str)
        plt.plot(mo.soc, mo.voc_stat, color='cyan', linestyle=':', label='voc_stat'+old_str)
        if mo.voc_soc is not None:
            plt.plot(mo.soc, mo.voc_soc, color='blue', linestyle='-', label='voc_soc'+old_str)
        plt.plot(mv.soc, mv.voc_soc, color='red', linestyle='--', label='voc_soc'+new_str)
        plt.plot(so.soc_s, so.voc_stat_s, color='green', linestyle='-.', label='voc_stat_s'+old_str)
        if hasattr(sv, 'voc_stat'):
            plt.plot(sv.soc, sv.voc_stat, color='orange', linestyle=':', label='voc_stat_s'+new_str)
        elif hasattr(sv, 'voc_stat_s'):
            plt.plot(sv.soc_s, sv.voc_stat_s, color='orange', linestyle=':', label='voc_stat_s'+new_str)
        plt.plot(mo.soc, mo.vsat, color='red', linestyle='-', label='vsat'+old_str)
        plt.plot(mv.soc, mv.vsat, color='black', linestyle='--', label='vsat'+new_str)
        plt.plot(mv.soc, mv.voc_stat, color='orange', linestyle='--', label='voc_stat'+new_str)
        plt.legend(loc=1)
        plt.subplot(323)
        if mo.voc_soc is not None:
            plt.plot(mo.time, mo.voc_soc-mo.voc_stat, color='blue', linestyle='-', label='e_wrap = voc_soc - voc_stat'+old_str)
        plt.plot(mv.time, np.array(mv.voc_soc)-np.array(mv.voc_stat), color='red', linestyle='--', label='e_wrap = voc_soc - voc_stat' + new_str)
        plt.legend(loc=1)
        plt.subplot(324)
        if mo.voc_soc is not None:
            plt.plot(mo.time, mo.voc_soc, color='blue', linestyle='-', label='voc_soc'+old_str)
            plt.plot(mo.time, mo.voc_stat, color='red', linestyle='--', label='voc_stat' + old_str)
        plt.plot(so.time, so.voc_stat_s, color='magenta', linestyle='-.', label='voc_stat_s' + old_str)
        plt.plot(mv.time, mv.voc_soc, color='green', linestyle='--', label='voc_soc' + new_str)
        plt.plot(mv.time, mv.voc_stat, color='cyan', linestyle='-.', label='voc_stat' + new_str)
        plt.plot(smv.time, smv.voc_stat_s, color='orange', linestyle=':', label='voc_stat_s' + new_str)
        plt.legend(loc=1)
        plt.subplot(325)
        if mo.vb_h is not None and max(mo.vb_h) > 1.:
            plt.plot(mo.time, mo.vb_h, color='magenta', linestyle=':', label='vb_hdwe'+old_str)
        plt.plot(mo.time, mo.voc_stat, color='cyan', linestyle=':', label='voc_stat'+old_str)
        if mo.voc_soc is not None:
            plt.plot(mo.time, mo.voc_soc, color='blue', linestyle='-', label='voc_soc'+old_str)
        plt.plot(mv.time, mv.voc_soc, color='red', linestyle='--', label='voc_soc'+new_str)
        plt.plot(so.time, so.voc_stat_s, color='green', linestyle='-.', label='voc_stat_s'+old_str)
        if hasattr(sv, 'voc_stat'):
            plt.plot(sv.time, sv.voc_stat, color='orange', linestyle=':', label='voc_stat_s'+new_str)
        elif hasattr(sv, 'voc_stat_s'):
            plt.plot(sv.time, sv.voc_stat_s, color='orange', linestyle=':', label='voc_stat_s'+new_str)
        plt.plot(mo.time, mo.vsat, color='red', linestyle='-', label='vsat'+old_str)
        plt.plot(mv.time, mv.vsat, color='black', linestyle='--', label='vsat'+new_str)
        plt.plot(mv.time, mv.voc_stat, color='orange', linestyle='-.', label='voc_stat'+new_str)
        plt.legend(loc=1)
        plt.subplot(326)
        if mo.voc_soc is not None:
            plt.plot(mo.soc, mo.voc_soc, color='blue', linestyle='-', label='voc_soc'+old_str)
            plt.plot(mo.soc, mo.voc_stat, color='red', linestyle='-', label='voc_stat' + old_str)
        plt.legend(loc=1)

        plt.figure()  # sim_s  4
        n_fig += 1
        plt.subplot(221)
        plt.title(plot_title + ' sim_s 4')
        plt.plot(mo.time, mo.soc, color='blue', linestyle='-', label='soc'+old_str)
        plt.plot(mv.time, mv.soc, color='red', linestyle='--', label='soc'+new_str)
        plt.plot(so.time, so.soc_s, color='green', linestyle='-.', label='soc_s'+old_str)
        if hasattr(sv, 'soc'):
            plt.plot(sv.time, sv.soc, color='orange', linestyle=':', label='sv.soc_s'+new_str)
        elif hasattr(sv, 'soc_s'):
            plt.plot(sv.time, sv.soc_s, color='orange', linestyle=':', label='sv.soc_s'+new_str)
        plt.plot(mo.time, mo.soc_ekf, color='cyan', linestyle='-', label='soc_ekf'+old_str)
        plt.plot(mv.time, mv.soc_ekf, color='magenta', linestyle='--', label='soc_ekf'+new_str)
        plt.legend(loc=1)
        plt.subplot(222)
        plt.plot(mo.soc, mo.voc_stat, color='blue', linestyle='-', label='voc_stat(soc) = z '+old_str)
        plt.plot(mv.soc, mv.voc_stat, color='red', linestyle='--', label='voc_stat(soc) = z '+new_str)
        plt.plot(mo.soc, mo.voc_ekf, color='green', linestyle='-.', label='voc_ekf(soc) = Hx '+old_str)
        if mo.voc_soc is not None:
            plt.plot(mo.soc, mo.voc_soc, color='green', linestyle=':', label='voc_soc'+old_str)
        plt.plot(mv.soc, mv.voc_soc, color='orange', linestyle='-', label='voc_soc'+new_str)
        if mo.vb_h is not None:
            plt.plot(mo.soc, mo.vb_h, color='blue', linestyle='-', label='vb'+old_str)
        if hasattr(sv, 'voc_stat'):
            plt.plot(sv.soc, sv.voc_stat, color='red', linestyle=':', label='voc_stat_s'+new_str)
        elif hasattr(sv, 'voc_stat_s'):
            plt.plot(sv.soc_s, sv.voc_stat_s, color='red', linestyle=':', label='voc_stat_s'+new_str)
        plt.ylim(12.5, 14.5)
        plt.legend(loc=1)
        plt.subplot(223)
        plt.plot(mo.soc, mo.voc_stat, color='magenta', linestyle='-', label='voc_stat(soc) = z'+old_str)
        plt.plot(mv.soc, mv.voc_stat, color='black', linestyle='--', label='voc_stat(soc) = z'+new_str)
        plt.plot(mv.soc, mv.voc_ekf, color='cyan', linestyle=':', label='voc_ekf(soc) = Hx'+new_str)
        if mo.vb_h is not None:
            plt.plot(mo.soc, mo.vb_h, color='blue', linestyle='-', label='vb'+old_str)
        if hasattr(sv, 'voc_stat'):
            plt.plot(sv.soc, sv.voc_stat, color='red', linestyle=':', label='voc_stat_s'+new_str)
        elif hasattr(sv, 'voc_stat_s'):
            plt.plot(sv.soc_s, sv.voc_stat_s, color='red', linestyle=':', label='voc_stat_s'+new_str)
        plt.ylim(12.5, 14.5)
        plt.legend(loc=1)
        plt.subplot(224)
        plt.plot(mo.time, mo.voc_stat, color='magenta', linestyle='-', label='voc_stat(soc) = z'+old_str)
        plt.plot(mo.time, mo.voc_ekf, color='cyan', linestyle='--', label='voc_ekf(soc) = Hx'+old_str)
        if mo.voc_soc is not None:
            plt.plot(mo.time, mo.voc_soc, color='green', linestyle='-.', label='voc_soc'+old_str)
        plt.plot(mv.time, mv.voc_soc, color='orange', linestyle=':', label='voc_soc'+new_str)
        if mo.vb_h is not None:
            plt.plot(mo.time, mo.vb_h, color='blue', linestyle='-', label='vb'+old_str)
        plt.plot(mv.time, mv.voc_stat, color='black', linestyle=':', label='voc_stat(soc) = z '+new_str)
        plt.plot(mv.time, mv.voc_ekf, color='cyan', linestyle=':', label='voc_ekf(soc) = Hx '+new_str)
        if hasattr(sv, 'voc_stat'):
            plt.plot(sv.time, sv.voc_stat, color='red', linestyle=':', label='voc_stat_s'+new_str)
        elif hasattr(sv, 'voc_stat_s'):
            plt.plot(sv.time, sv.voc_stat_s, color='red', linestyle=':', label='voc_stat_s'+new_str)
        plt.ylim(12.5, 14.5)
        plt.legend(loc=1)
    return n_fig, fig_files
