# Battery - general purpose battery class for modeling
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

import numpy as np


class StateSpace:
    # Classical canonical state space time simulation

    def __init__(self, n=0, p=0, q=0):
        self.n = n
        self.p = p
        self.q = q
        self.u = np.zeros(shape=p)
        self.A = np.zeros(shape=(n, n))
        self.B = np.zeros(shape=(n, p))
        self.C = np.zeros(shape=(q, n))
        self.D = np.zeros(shape=(q, p))
        self.y = np.zeros(shape=q)
        self.x = np.zeros(shape=n)
        self.x_dot = self.x
        self.x_past = self.x
        self.dt = 0.
        self.saved = Saved(n, p, q)

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix + "StateSpace:\n"
        s += "  A = \n {}\n".format(self.A)
        s += "  x = {}\n".format(self.x)
        s += "  B = \n {}\n".format(self.B)
        s += "  u = {}\n".format(self.u)
        s += "  C = \n {}\n".format(self.C)
        s += "  D = \n {}\n".format(self.D)
        s += "  x_dot = {}\n".format(self.x_dot)
        s += "  y = {}\n".format(self.y)
        return s

    def calc_x_dot(self, u):
        self.u = u
        self.x_dot = self.A @ self.x + self.B @ self.u
        # Ax = self.A@self.x
        # Bu = self.B@self.u

    def init_state_space(self, x_init):
        self.x = np.array(x_init)
        self.x_past = self.x
        self.x_dot = self.x * 0.

    def save(self, time):
        self.saved.time = np.append(self.saved.time, time)
        self.saved.time_min = np.append(self.saved.time_min, time / 60.)
        self.saved.time_day = np.append(self.saved.time_day, time / 3600. / 24.)
        self.saved.y = np.append(self.saved.y, self.y)
        if self.saved.u != []:
            self.saved.u = np.append(self.saved.u, self.u.reshape(1, 2), axis=0)
            self.saved.x = np.append(self.saved.x, self.x.reshape(1, 2), axis=0)
            self.saved.x_dot = np.append(self.saved.x_dot, self.x_dot.reshape(1, 2), axis=0)
            self.saved.x_past = np.append(self.saved.x_past, self.x_past.reshape(1, 2), axis=0)
        else:
            self.saved.u = self.u.reshape(1, 2)
            self.saved.x = self.x.reshape(1, 2)
            self.saved.x_dot = self.x_dot.reshape(1, 2)
            self.saved.x_past = self.x_past.reshape(1, 2)

    def update(self, dt):
        if dt is not None:
            self.dt = dt
        self.x_past = self.x.copy()  # Backwards Euler has extra delay
        self.x += self.x_dot * self.dt
        self.y = self.C @ self.x_past + self.D @ self.u  # uses past (backward Euler)
        return self.y


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self, n=0, p=0, q=0):
        # self.time = np.zeros(shape=1)
        # self.time_min = np.zeros(shape=1)
        # self.time_day = np.zeros(shape=1)
        # self.n = n
        # self.p = p
        # self.q = q
        # self.u = np.zeros(shape=(1, p))
        # self.y = np.zeros(shape=q)
        # self.x = np.zeros(shape=(1, n))
        # self.x_dot = self.x
        # self.x_past = self.x
        self.time = []
        self.time_min = []
        self.time_day = []
        self.n = []
        self.p = []
        self.q = []
        self.u = []
        self.y = []
        self.x = []
        self.x_dot = []
        self.x_past = []


if __name__ == '__main__':
    import sys
    import doctest
    doctest.testmod(sys.modules['__main__'])

    def construct_state_space_monitor():
        r0 = 0.003  # Randles R0, ohms
        tau_ct = 0.2  # Randles charge transfer time constant, s (=1/Rct/Cct)
        rct = 0.0016  # Randles charge transfer resistance, ohms
        tau_dif = 83  # Randles diffusion time constant, s (=1/Rdif/Cdif)
        r_dif = 0.0077  # Randles diffusion resistance, ohms
        c_ct = tau_ct / rct
        c_dif = tau_dif / r_dif
        print('-1/Rc/Cc=', -1/tau_ct, '-1/Rd/Cd=', -1/tau_dif)
        print('1/Cc=', 1/c_ct, '1/Cd=', 1/c_dif)
        print('r0=', r0)
        a = np.array([[-1 / tau_ct, 0],
                      [0, -1 / tau_dif]])
        b = np.array([[1 / c_ct,   0],
                      [1 / c_dif,  0]])
        c = np.array([-1., -1])
        d = np.array([-r0, 1])
        return a, b, c, d

    def construct_state_space_model():
        r0 = 0.003  # Randles R0, ohms
        tau_ct = 0.2  # Randles charge transfer time constant, s (=1/Rct/Cct)
        rct = 0.0016  # Randles charge transfer resistance, ohms
        tau_dif = 83  # Randles diffusion time constant, s (=1/Rdif/Cdif)
        r_dif = 0.0077  # Randles diffusion resistance, ohms
        c_ct = tau_ct / rct
        c_dif = tau_dif / r_dif
        print('-1/Rc/Cc=', -1/tau_ct, '-1/Rd/Cd=', -1/tau_dif)
        print('1/Cc=', 1/c_ct, '1/Cd=', 1/c_dif)
        print('r0=', r0)
        a = np.array([[-1 / tau_ct, 0],
                      [0, -1 / tau_dif]])
        b = np.array([[1 / c_ct,   0],
                      [1 / c_dif,  0]])
        c = np.array([1., 1])
        d = np.array([r0, 1])
        return a, b, c, d

    def main():
        ss = StateSpace(2, 2, 1)
        ss.A, ss.B, ss.C, ss.D = construct_state_space_monitor()
        print('Monitor::')
        print('A=', ss.A)
        print('B=', ss.B)
        print('C=', ss.C)
        print('D=', ss.D)
        ss.A, ss.B, ss.C, ss.D = construct_state_space_model()
        print('Model::')
        print('A=', ss.A)
        print('B=', ss.B)
        print('C=', ss.C)
        print('D=', ss.D)


    main()
