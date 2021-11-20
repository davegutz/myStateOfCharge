# GP_battery - general purpose battery class
# Copyright (C) 2021 Dave Gutz
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

"""Define a general purpose battery model including Randle's model and SoC-VOV model as well as Kalman filtering
support for simplified Mathworks' tracker. See Huria, Ceraolo, Gazzarri, & Jackey, 2013 Simplified Extended Kalman
Filter Observer for SOC Estimation of Commercial Power-Oriented LFP Lithium Battery Cells """

import numpy as np
import math
from pyDAGx.lookup_table import LookupTable


class Battery:
    # Battery model:  Randle's dynamics, SOC-VOC model, EKF support

    def __init__(self, n_cells=4, r0=0.003, tau_ct=3.7, rct=0.0016, tau_dif=83, rdif=0.0077, dt_=0.1, b=0., a=0.,
                 c=0., n=0.4, m=0.478, d=0.707, t_t=None, t_b=None, t_a=None, t_c=None, nom_batt_cap=100.,
                 true_batt_cap=102., nom_sys_volt=13., dv=0, sr=1):
        """ Default values from Taborelli & Onori, 2013, State of Charge Estimation Using Extended Kalman Filters for
        Battery Management System.   Battery equations from LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang
        etal.pdf.'  SOC-OCV curve fit './Battery State/BattleBorn Rev1.xls:Model Fit' using solver with min slope
        constraint >=0.02 V/soc.  m and n using Zhang values.   Had to scale soc because  actual capacity > NOM_BATT_CAP
        so equations error when soc<=0 to match data.
        """
        # Dynamics
        self.n_cells = n_cells
        self.r_0 = r0 * n_cells
        self.tau_ct = tau_ct
        self.r_ct = rct * n_cells
        self.c_ct = self.tau_ct / self.r_ct
        self.tau_dif = tau_dif
        self.r_dif = rdif * n_cells
        self.c_dif = self.tau_dif / self.r_dif
        self.A, self.B, self.C, self.D = self.construct_state_space()
        self.u = np.array([0., 0]).T
        self.dt = dt_
        self.x = None
        self.x_past = None
        self.x_dot = self.x
        self.y = np.array([0., 0]).T
        # SOC-VOC
        # Battery coefficients
        self.b = b
        self.a = a
        self.c = c
        self.d = d
        self.n = n
        self.m = m
        if t_b is None:
            t_t = [0., 25., 50.]
            t_b = [-0.836, -0.836, -0.836]
            t_a = [3.999, 4.046, 4.093]
            t_c = [-1.181, -1.181, -1.181]
        self.lut_b = LookupTable()
        self.lut_a = LookupTable()
        self.lut_c = LookupTable()
        self.lut_b.addAxis('T_degC', t_t)
        self.lut_a.addAxis('T_degC', t_t)
        self.lut_c.addAxis('T_degC', t_t)
        self.lut_b.setValueTable(t_b)
        self.lut_a.setValueTable(t_a)
        self.lut_c.setValueTable(t_c)
        # Other things
        self.socu = 0.  # State of charge, unscaled, %
        self.socs = 0.  # State of charge, scaled, %
        self.dV_dSoc_u = 0.  # Slope of soc-voc curve, unscaled, V/%
        self.dV_dSoc_s = 0.  # Slope of soc-voc curve, scaled, V/%
        self.nom_batt_cap = nom_batt_cap
        """ Nominal battery label capacity, e.g. 100, Ah. Accounts for internal losses.
            This is what gets delivered, e.g. Wshunt/NOM_SYS_VOLT.  Also varies 0.2-0.4C
            currents or 20-40 A for a 100 Ah battery"""
        self.nom_sys_volt = nom_sys_volt  # Nominal battery label voltage, e.g. 12, V
        self.vsat = nom_sys_volt + 1.7  # Saturation voltage, V
        self.voc = nom_sys_volt  # Nominal battery storage voltage, V
        self.sat = True  # Battery is saturated, T/F
        self.ib = 0  # Current into battery post, A
        self.vb = nom_sys_volt  # Battery voltage at post, V
        self.ioc = 0  # Current into battery storage, A
        self.cu = nom_batt_cap  # Assumed capacity, Ah
        self.cs = true_batt_cap  # Data fit to this capacity to avoid math 0, Ah
        self.dv = dv  # Adjustment for voltage level, V (0)
        self.sr = sr  # Adjustment for resistance scalar, fraction (1)
        self.pow_in = None  # Charging power, W
        self.time_charge = None  # Charging time to 100%, hr

    def construct_state_space(self):
        """ State-space representation of dynamics
        Inputs:   Ib - current at Vbatt = Vb = current at shunt, A
                  Vocv - internal open circuit voltage, V
        Outputs:  Vb - voltage at positive, V
                  Iocv  - current into storage, A
        States:   Vbc - RC Vb-Vc, V
                  Vcd - RC Vc-Vd, V
        Other:    Vc - voltage downstream of charge transfer model, ct-->c
                  Vd - voltage downstream of diffusion process model, dif-->d
        """
        a = np.array([[-1 / self.tau_ct, 0],
                      [0, -1 / self.tau_dif]])
        b = np.array([[1 / self.c_ct, 0],
                      [1 / self.c_dif, 0]])
        c = np.array([[1, 1],
                      [0, 0]])
        d = np.array([[self.r_0, 1],
                      [1, 0]])
        return a, b, c, d

    def propagate_state_space(self, u_, dt_=None):
        # Time propagation from state-space using backward Euler
        self.u = u_
        self.ib = self.u[0]
        self.voc = self.u[1]
        if self.x is None:  # Initialize
            self.x = np.array([self.ib * self.r_ct, self.ib * self.r_dif]).T
        if dt_ is not None:
            self.dt = dt_
        self.x_past = self.x
        self.x_dot = self.A @ self.x + self.B @ self.u
        self.x += self.x_dot * self.dt
        self.y = self.C @ self.x_past + self.D @ self.u  # uses past (backward Euler)
        self.vb = self.y[0]
        self.ioc = self.y[1]

    # SOC-OCV curve fit method per Zhang, et al
    def calculate(self, t_c=25., socu_fraction=1., ib=0., dt_=None):
        eps_max = 1 - 1e-6  # Numerical maximum of coefficient model with scaled socs.
        eps_min = 1e-6  # Numerical minimum of coefficient model without scaled socs.
        eps_max_unscaled = 1 - 1e-6  # Numerical maximum of coefficient model with scaled socs.
        eps_min_unscaled = 1 - (
                    1 - eps_min) * self.cs / self.cu  # Numerical minimum of coefficient model without scaled socs.

        if dt_ is not None:
            self.dt = dt_
        self.b, self.a, self.c = self.look(t_c)
        self.socu = socu_fraction
        self.socs = 1. - (1. - self.socu) * self.cu / self.cs
        socs_lim = max(min(self.socs, eps_max), eps_min)

        # Perform computationally expensive steps one time
        log_socs = math.log(socs_lim)
        exp_n_socs = math.exp(self.n * (socs_lim - 1))
        pow_log_socs = math.pow(-log_socs, self.m)

        # VOC-OCV model
        self.dV_dSoc_s = float(self.n_cells) * (
                    self.b * self.m / socs_lim * pow_log_socs / log_socs + self.c + self.d * self.n * exp_n_socs)
        self.dV_dSoc_u = self.dV_dSoc_s * self.cu / self.cs
        self.voc = float(self.n_cells) * (self.a + self.b * pow_log_socs + self.c * socs_lim + self.d * exp_n_socs) \
                   + (self.socs - socs_lim) * self.dV_dSoc_s  # slightly beyond
        self.voc += self.dv  # Experimentally varied
        #  self.d2v_dsocs2 = float(self.n_cells) * ( self.b*self.m/self.soc/self.soc*pow_log_socs/log_socs*((self.m-1.)/log_socs - 1.) + self.d*self.n*self.n*exp_n_socs )

        # Dynamics
        self.propagate_state_space(u, dt_=dt)

        # Summarize
        self.pow_in = self.vb * self.ib - self.ib * self.ib * (
                    self.r0 + self.r_dif + self.r_ct) * self.sr  # Internal resistance of battery is a loss
        if self.pow_in > 1.:
            self.time_charge = min(self.nom_batt_cap / self.pow_in * self.nom_sys_volt * (1. - socs_lim),
                                   24.)  # nom_batt_cap is defined at nom_sys_volt
        elif self.pow_in < -1.:
            self.time_charge = max(self.nom_batt_cap / self.pow_in * self.nom_sys_volt * socs_lim,
                                   -24.)  # nom_batt_cap is defined at nom_sys_volt
        elif self.pow_in >= 0.:
            self.time_charge = 24. * (1. - socs_lim)
        else:
            self.time_charge = -24. * socs_lim
        self.vsat = self.nom_vsat + (t_c - 25.) * self.dvoc_dt
        self.sat = self.voc >= self.vsat

        return self.vb, self.dv_dsocs

    def look(self, t_c):
        b = self.lut_b.lookup(T_degC=t_c)
        a = self.lut_a.lookup(T_degC=t_c)
        c = self.lut_c.lookup(T_degC=t_c)
        return b, a, c

    def vbc(self):
        return self.x[0]

    def vcd(self):
        return self.x[1]

    def vbc_dot(self):
        return self.x_dot[0]

    def vcd_dot(self):
        return self.x_dot[1]

    def i_c_ct(self):
        return self.vbc_dot() * self.c_ct

    def i_c_dif(self):
        return self.vcd_dot() * self.c_dif

    def i_r_ct(self):
        return self.vbc() / self.r_ct

    def i_r_dif(self):
        return self.vcd() / self.r_dif

    def vd(self):
        return self.voc + self.ioc * self.r_0

    def vc(self):
        return self.vd() + self.vcd()


if __name__ == '__main__':
    import sys
    import doctest

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt
    import book_format

    book_format.set_style()

    # coefficient definition
    dt = 0.1
    battery_model = Battery(n_cells=4)

    # time_end = 200
    time_end = 200
    t = np.arange(0, time_end + dt, dt)
    n = len(t)

    Is = []
    Vocs = []
    Vbs = []
    Vcs = []
    Vds = []
    Iocs = []
    I_Ccts = []
    I_Rcts = []
    I_Cdifs = []
    I_Rdifs = []
    Vbc_dots = []
    Vcd_dots = []
    for i in range(len(t)):
        if t[i] < 1:
            u = np.array([0, battery_model.nom_sys_volt]).T
        else:
            u = np.array([30., battery_model.nom_sys_volt]).T

        battery_model.propagate_state_space(u, dt_=dt)

        Is.append(battery_model.ib)
        Vocs.append(battery_model.voc)
        Vbs.append(battery_model.vb)
        Vcs.append(battery_model.vc())
        Vds.append(battery_model.vd())
        Iocs.append(battery_model.ioc)
        I_Ccts.append(battery_model.i_c_ct())
        I_Cdifs.append(battery_model.i_c_dif())
        I_Rcts.append(battery_model.i_r_ct())
        I_Rdifs.append(battery_model.i_r_dif())
        Vbc_dots.append(battery_model.vbc_dot())
        Vcd_dots.append(battery_model.vcd_dot())

    plt.figure()
    plt.subplot(311)
    plt.title('GP_battery.py')
    plt.plot(t, Is, color='green', label='I')
    plt.plot(t, I_Rcts, color='red', label='I_Rct')
    plt.plot(t, I_Cdifs, color='cyan', label='I_Cdif')
    plt.plot(t, I_Rdifs, color='orange', linestyle='--', label='I_Rdif')
    plt.plot(t, Iocs, color='orange', label='Ioc')
    plt.legend(loc=1)
    plt.subplot(312)
    plt.plot(t, Vbs, color='green', label='Vb')
    plt.plot(t, Vcs, color='blue', label='Vc')
    plt.plot(t, Vds, color='red', label='Vd')
    plt.plot(t, Vocs, color='blue', label='Voc')
    plt.legend(loc=1)
    plt.subplot(313)
    plt.plot(t, Vbc_dots, color='green', label='Vbc_dot')
    plt.plot(t, Vcd_dots, color='blue', label='Vcd_dot')
    plt.legend(loc=1)
    plt.show()
