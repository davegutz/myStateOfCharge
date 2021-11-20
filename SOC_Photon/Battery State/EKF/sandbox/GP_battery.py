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


class Battery:
    # Dynamic Battery model:  Randle's dynamics, SOC-VOC model, EKF support

    def __init__(self, n_cells=4, r0=0.003, tau_ct=3.7, rct=0.0016, tau_dif=83, rdif=0.0077, dt_=0.1):
        """ Default values from Taborelli & Onori, 2013, State of Charge Estimation Using Extended Kalman Filters for
        Battery Management System
        """
        self.n_cells = n_cells
        self.r_0 = r0*n_cells
        self.tau_ct = tau_ct
        self.r_ct = rct*n_cells
        self.c_ct = self.tau_ct/self.r_ct
        self.tau_dif = tau_dif
        self.r_dif = rdif*n_cells
        self.c_dif = self.tau_dif/self.r_dif
        self.A, self.B, self.C, self.D = self.dynamic_state_space()
        self.u = np.array([0., 0]).T
        self.dt = dt_
        self.x = np.array([0., 0]).T
        self.x_past = self.x
        self.x_dot = self.x
        self.y = np.array([0., 0]).T

    def dynamic_state_space(self):
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
        a = np.array([[-1/self.tau_ct, 0],
                      [0,              -1/self.tau_dif]])
        b = np.array([[1/self.c_ct,    0],
                      [1/self.c_dif,   0]])
        c = np.array([[1,              1],
                      [0,              0]])
        d = np.array([[self.r_0,       1],
                      [1,              0]])
        return a, b, c, d

    def propagate_state_space(self, u_, dt_=None):
        # Time propagation from state-space using backward Euler
        self.u = u_
        self.x_past = self.x
        if dt_ is not None:
            self.dt = dt_
        self.x_dot = self.A @ self.x + self.B @ self.u
        self.x += self.x_dot * self.dt
        self.y = self.C @ self.x_past + self.D @ self.u # uses past (backward Euler)

    def ib(self): return self.u[0]
    def voc(self): return self.u[1]
    def vbc(self): return self.x[0]
    def vcd(self): return self.x[1]
    def vb(self): return self.y[0]
    def ioc(self): return self.y[1]
    def vbc_dot(self): return self.x_dot[0]
    def vcd_dot(self): return self.x_dot[1]
    def i_c_ct(self): return self.vbc_dot()*self.c_ct
    def i_c_dif(self): return self.vcd_dot()*self.c_dif
    def i_r_ct(self): return self.vbc()/self.r_ct
    def i_r_dif(self): return self.vcd()/self.r_dif
    def vd(self): return self.voc()+self.ioc()*self.r_0
    def vc(self): return self.vd()+self.vcd()


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
    t = np.arange(0, time_end+dt, dt)
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
            I = 0.0
            Voc = 13.5
            u = np.array([I, Voc]).T
        else:
            I = 30.0
            Voc = 13.5
            u = np.array([I, Voc]).T

        battery_model.propagate_state_space(u, dt_=dt)

        Is.append(battery_model.ioc())
        Vocs.append(battery_model.voc())
        Vbs.append(battery_model.vb())
        Vcs.append(battery_model.vc())
        Vds.append(battery_model.vd())
        Iocs.append(battery_model.ioc())
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
