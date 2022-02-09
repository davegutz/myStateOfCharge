# Battery - general purpose battery class for modeling
# Copyright (C) 2021 Dave Gutz
#
# This library is free software; you can redistribute it and/or
# modify it under the terss of the GNU Lesser General Public
# License as published by the Free Software Foundation;
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# See http://www.fsf.org/licensing/licenses/lgpl.txt for full license text.

"""Define a general purpose battery model including Randle's model and SoC-VOV model."""

import numpy as np
import math
from StateSpace import StateSpace
import matplotlib.pyplot as plt


# Battery constants


class BatteryHeat:
    # Battery heat model:  discrete lumped parameter heat flux modeling warmup of Battleborn using 36 W of heat pad

    def __init__(self, temp_c, hi0=1., n=5, M=29., l=12./39., w=7./39., h=9./39., cv=0.075, hij=0.23, rho=2230.):
        """ Default values  from various for Battleborn 100 Ah LFP battery
        hi0 = 1 W/m^2/C (R1 insulation old 1/2 inch sleeping pad)
        M = 29  kg mass of battery elements
        l = 12/39, m
        w = 7/39, m
        h = 9/39, m
        cv = 541 J/C * 0.5 kg / 3600 s/hr = 0.075 W-hr/kg (1 J = 1 W-s)
        rho = 0.5 kg / 224 ml = 2230 kg/m^3
        hij = 0.23 W/K approximate admittance of entire battery thickness
        """

        # Defaults
        assert (n % 2) != 0  # n must be odd to place Tb in center
        assert (n >= 3)  # n must be 3 or larger unless you want to rewrite the state equations

        self.n = n
        Ae = l * w
        Ai = (2.*(l*h) + 2.*(h*w) ) / n
        V = l * w * h
        M = V * rho
        self.T0 = temp_c
        self.W = 0
        self.Qn = self.W
        self.Ti = np.zeros(shape=n)
        for i in np.arange(0, n):
            self.Ti[i] = temp_c
        self.Tbs = temp_c
        self.mi = M / float(n)
        self.Ci = cv * self.mi
        self.Hij = Ae * hij / float(n)
        self.Hi0 = hi0 * Ai
        self.He0 = l*w*hi0
        self.Hin = 1. / (1./(self.Hij/2.) + 1./self.He0)
        He0ij = 1. / (self.He0/(self.Hij/2.) + 1.)
        Hije0 = 1. / (1. + (self.Hij / 2.) / self.He0)
        self.i_Tb = int(n / 2)
        self.Tb = self.Ti[self.i_Tb]
        self.Tbs = temp_c
        self.Tw = temp_c
        self.Tn = temp_c

        # Build ss arrays
        # first slice
        self.ss_model = StateSpace(n, 2, 3)  # n slices, [T0, W] --> [Tbs, Tb, Tw]
        self.ss_model.A[0,0] = (-self.Hij/2. - self.Hin - self.Hi0) / self.Ci
        self.ss_model.A[0,1] = self.Hij/2./self.Ci
        self.ss_model.B[0,0] = He0ij /self.Ci
        self.ss_model.B[0,1] = (self.Hi0 + self.Hin) / self.Ci
        # between slices
        for i in np.arange(1, n-1):
            self.ss_model.A[i,i-1] = self.Hij/2. / self.Ci
            self.ss_model.A[i,i] = (-self.Hi0 - self.Hij) / self.Ci  # note Hij not divided by 2
            self.ss_model.A[i,i+1] = self.ss_model.A[i,i-1]
            self.ss_model.B[i,1] = self.Hi0 / self.Ci
        self.ss_model.C[1,self.i_Tb] = 1
        # last slice
        self.ss_model.A[n-1,n-2] = self.Hij / 2. / self.Ci
        self.ss_model.A[n-1,n-1] = (-self.Hi0 - self.Hij / 2. - self.Hin) / self.Ci
        self.ss_model.B[n-1,1] = (self.Hin + self.Hi0) / self.Ci
        self.ss_model.C[0,n-1] = 1. - Hije0
        self.ss_model.C[2,0] = self.ss_model.C[0,n-1]
        self.ss_model.D[0,1] = -Hije0
        self.ss_model.D[2,0] = 1. / ( self.He0 + self.Hij/2. )
        self.ss_model.D[2,1] = -self.ss_model.D[0,1]

        self.saved = Saved(n=self.n, i_Tb=self.i_Tb)  # for plots and prints

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix + "BatteryHeat:\n"
        s += "  mi  = {:7.3f}  // mass of slice, kg\n".format(self.mi)
        s += "  Ci  = {:7.4f}  // Heat capacity of slice, W-hr\n".format(self.Ci)
        s += "  Hij = {:8.6f} // Heat transfer rate of slice (=A/R), W/deg C\n".format(self.Hij)
        s += "  Hi0 = {:7.4f}  // Heat transfer rate of slice to atm (=A/R), W/deg C\n".format(self.Hi0)
        s += "  He0 = {:7.4f}  // Heat transfer rate of top or bottom to atm (=A/R), W/deg C\n".format(self.He0)
        s += '  W   = {:5.1f}, // Power in, W\n'.format(self.W)
        s += '  Qn  = {:5.1f}, // Heat out, W\n'.format(self.Qn)
        s += '  T0  = {:5.1f}, // Ambient temp, deg C\n'.format(self.T0)
        s += '  Tw  = {:5.1f}, // Heat pad wire temp, deg C\n'.format(self.Tw)
        s += '  Ti  =  ['
        for i in np.arange(0, self.n):
            s += ' {:5.1f}'.format(self.Ti[i])
            if i<self.n-1:
                s += ','
        s += '  ]  // Slice temps, deg C\n'
        s += '  i_Tb= {:d},  // Slice location of battery bulk temp\n'.format(self.i_Tb)
        s += '  Tb  = {:5.1f}, // Battery bulk temp, deg C\n'.format(self.Tb)
        s += '  Tbs = {:5.1f}, // Battery temp sense, deg C\n'.format(self.Tbs)
        s += "\n  "
        s += self.ss_model.__str__(prefix + 'Battery:')
        return s

    def assign_temp_c(self, temp_c):
        self.T0 = temp_c
        self.Tw = temp_c
        self.Tbs = temp_c
        self.Tb = temp_c
        for i in np.arange(0, self.n):
            self.Ti[i] = temp_c

    def calculate(self, temp_c, W, dt):
        self.T0 = temp_c
        self.W = W

        # Dynamics
        u = np.array([self.W, self.T0]).T
        self.ss_model.calc_x_dot(u)
        self.ss_model.update(dt)
        self.Tbs, self.Tb, self.Tw = self.ss_model.y
        self.Ti = self.ss_model.x
        self.Qn = (self.Ti[self.n-1] - self.T0) * self.Hin

        return

    def save(self, time, T_ref):
        self.saved.time.append(time)
        self.saved.T_Ref.append(T_ref)
        self.saved.T0.append(self.T0)
        self.saved.W.append(self.W)
        self.saved.Tw.append(self.Tw)
        self.saved.T1.append(self.Ti[0])
        self.saved.Tb.append(self.Tb)
        self.saved.Tn.append(self.Ti[self.n-1])
        self.saved.Tbs.append(self.Tbs)
        self.saved.Qn.append(self.Qn)


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self, n, i_Tb):
        self.n = n
        self.i_Tb = i_Tb
        self.time = []
        self.T_Ref = []
        self.T0 = []
        self.W = []
        self.Tw = []
        self.T1 = []
        self.Tb = []
        self.Tn = []
        self.Tbs = []
        self.Qn = []


def overall(ss, filename, fig_files=None, plot_title=None, n_fig=0):
    if fig_files is None:
        fig_files = []
    plt.figure()
    n_fig += 1
    plt.subplot(211)
    plt.title(plot_title)
    plt.plot(ss.time, ss.W, color='black', label='W in')
    plt.plot(ss.time, ss.Qn, color='blue', label='Q out')
    plt.legend(loc=1)
    plt.subplot(212)
    plt.plot(ss.time, ss.Tw, color='red', label='Tw')
    plt.plot(ss.time, ss.T1, color='red', linestyle='dashed', label='T1')
    plt.plot(ss.time, ss.Tb, color='green', label='Tb')
    plt.plot(ss.time, ss.Tn, color='cyan', label='Tn')
    plt.plot(ss.time, ss.T_Ref, color='blue', linestyle='dotted', label='T_Ref')
    plt.plot(ss.time, ss.Tbs, color='blue', label='Tbs')
    plt.plot(ss.time, ss.T0, color='magenta', label='T0')
    plt.legend(loc=1)
    fig_file_name = filename + '_' + str(n_fig) + ".png"
    fig_files.append(fig_file_name)
    plt.savefig(fig_file_name, format="png")

    # plt.figure()
    # n_fig += 1
    # plt.subplot(111)
    # plt.title(plot_title)
    # plt.plot(ss.soc, ss.voc, color='black', linestyle='dotted', label='SIM voc vs soc')
    # plt.legend(loc=2)
    # fig_file_name = filename + "_" + str(n_fig) + ".png"
    # fig_files.append(fig_file_name)
    # plt.savefig(fig_file_name, format="png")

    return n_fig, fig_files
