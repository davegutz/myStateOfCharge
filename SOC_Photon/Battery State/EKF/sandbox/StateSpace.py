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
        self.u = np.zeros(shape=(n, p))
        self.A = np.zeros(shape=(n, n))
        self.B = np.zeros(shape=(n, p))
        self.C = np.zeros(shape=(q, n))
        self.D = np.zeros(shape=(q, p))
        self.y = np.zeros(shape=(q, 1))
        self.x = np.zeros(shape=(n, 1))
        self.x_dot = self.x
        self.x_past = self.x
        self.dt = 0.

    def calc_x_dot(self, u):
        self.u = u
        self.x_dot = self.A @ self.x + self.B @ self.u

    def update(self, reset, x_init, dt):
        if reset:
            self.x = x_init.T
        if dt is not None:
            self.dt = dt
        self.x_past = self.x
        self.x += self.x_dot * self.dt
        self.y = self.C @ self.x_past + self.D @ self.u  # uses past (backward Euler)
        return self.y


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
