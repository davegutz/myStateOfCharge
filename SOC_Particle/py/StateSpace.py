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
from numpy.linalg import inv
from myFilters import LagExp


class StateSpace:
    # Classical canonical state space time simulation

    def __init__(self, n=0, p=0, q=0):
        self.n = n
        self.p = p
        self.q = q
        self.u = np.zeros(shape=p)
        self.A = np.zeros(shape=(n, n))
        self.B = np.zeros(shape=(n, p))
        self.AinvB = np.zeros(shape=(n, p))
        self.C = np.zeros(shape=(q, n))
        self.D = np.zeros(shape=(q, p))
        self.y = np.zeros(shape=q)
        self.x = np.zeros(shape=n)
        self.x_dot = self.x
        self.x_past = self.x
        self.dt = 0.
        self.saved = Saved()

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
        s += "  AinvB = \n {}\n".format(self.AinvB)
        return s

    def calc_x_dot(self, u):
        self.u = u
        if len(self.A) > 1:
            self.x_dot = self.A @ self.x + self.B @ self.u
        else:
            self.x_dot = self.A * self.x + self.B * self.u

        # Ax = self.A@self.x
        # Bu = self.B@self.u

    def init_state_space(self, u_init):
        self.u = np.array(u_init)
        if len(self.A) > 1:
            self.AinvB = inv(self.A) @ self.B
            self.x = -self.AinvB @ self.u
            self.x_past = self.x
            self.calc_x_dot(self.u)
            self.y = self.C @ self.x_past + self.D @ self.u  # u
        else:
            self.AinvB = np.array(1/self.A)
            self.x = -self.AinvB * self.u
            self.x_past = self.x
            self.calc_x_dot(self.u)
            self.y = self.C * self.x_past + self.D * self.u  # u
        return

    def save(self, time):
        self.saved.time = np.append(self.saved.time, time)
        self.saved.time_min = np.append(self.saved.time_min, time / 60.)
        self.saved.time_day = np.append(self.saved.time_day, time / 3600. / 24.)
        self.saved.y = np.append(self.saved.y, self.y)
        if not self.saved.u:
            self.saved.u = self.u.reshape(1, 2)
            self.saved.x = self.x.reshape(1, 2)
            self.saved.x_dot = self.x_dot.reshape(1, 2)
            self.saved.x_past = self.x_past.reshape(1, 2)
        else:
            self.saved.u = np.append(self.saved.u, self.u.reshape(1, 2), axis=0)
            self.saved.x = np.append(self.saved.x, self.x.reshape(1, 2), axis=0)
            self.saved.x_dot = np.append(self.saved.x_dot, self.x_dot.reshape(1, 2), axis=0)
            self.saved.x_past = np.append(self.saved.x_past, self.x_past.reshape(1, 2), axis=0)

    def update(self, _dt, reset=False):
        if dt is not None:
            self.dt = _dt
        self.x_past = self.x.copy()  # Backwards Euler has extra delay
        if not reset:
            self.x += self.x_dot * self.dt
        if len(self.A) > 1:
            self.y = self.C @ self.x_past + self.D @ self.u  # uses past (backward Euler)
        else:
            self.y = self.C * self.x_past + self.D * self.u
        return self.y


class Saved:
    # For plot savings.   A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
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
    r_0 = 0.003
    r_ctOld = 0.0016
    r_ct = 0.0077
    tau_ct = 0.2
    tau_dif = 83
    c_ct = tau_ct / r_ctOld
    c_dif = tau_dif / r_ct
    dt = 0.1
    print('RcCc=', tau_ct, 'RdCd=', tau_dif)
    print('Cc=', c_ct, 'Cd=', c_dif)
    print('r_0=', r_0, 'r_ctOld=', r_ctOld, 'r_ct=', r_ct)
    print('dt=', dt)

    # ss model dim 1 where r_ctOld combined with r_0
    def construct_state_space_model_1():
        a = np.array([-1 / tau_dif])
        b = np.array([1. / c_dif])
        c = np.array([1])
        d = np.array([r_0 + r_ctOld])
        AinvB = b / a
        return a, b, c, d, AinvB

    def construct_state_space_model():
        print('-1/Rc/Cc=', -1/tau_ct, '-1/Rd/Cd=', -1/tau_dif)
        print('1/Cc=', 1/c_ct, '1/Cd=', 1/c_dif)
        print('r_0=', r_0)
        a = np.array([[-1 / tau_ct, 0],
                      [0, -1 / tau_dif]])
        b = np.array([[1 / c_ct,   0],
                      [1 / c_dif,  0]])
        c = np.array([1., 1])
        d = np.array([r_0, 1])
        AinvB = inv(a)*b
        return a, b, c, d, AinvB


    def construct_state_space_model_1l(dt_=0.1, tau_dif_=tau_dif, max_=100, min_=-100):
        one_order = LagExp(dt_, tau=tau_dif_, min_=min_, max_=max_)
        return one_order

    def main():

        import matplotlib.pyplot as plt
        plt.rcParams['axes.grid'] = True

        ss = StateSpace(2, 2, 1)
        ss.A, ss.B, ss.C, ss.D, ss.AinvB = construct_state_space_model()
        print('')
        print('Model::')
        print('A=', ss.A)
        print('B=', ss.B)
        print('C=', ss.C)
        print('D=', ss.D)
        print('AinvB=', ss.AinvB)
        ss.init_state_space([0., 0.])

        ss1 = StateSpace(1, 1, 1)
        ss1.A, ss1.B, ss1.C, ss1.D, ss1.AinvB = construct_state_space_model_1()
        ss1.init_state_space(0.)

        ssl = construct_state_space_model_1l(tau_dif)
        print('')
        print('Model_1l::')
        print('tau_dif', tau_dif)

        n = 12
        t = []
        inp_ = 0
        inp = []
        out = []
        out_1 = []
        out_1l = []
        print('    t, reset, in,    out2x, out_1, out_1l')
        s = ""
        for i in range(n):
            t.append(i*dt)
            reset = t[i] == 0.
            if t[i] > dt:
                inp_ = -75./0.4*t[i]
            inp.append(inp_)
            ss.calc_x_dot([inp_, 0])
            ss.update(dt, reset)
            ss1.calc_x_dot(inp_)
            ss1.update(dt, reset)
            out.append(ss.y)
            out_1.append(ss1.y[0])
            dv_dyn = ssl.calculate(inp_, reset, dt)*r_ct + inp_*(r_0 + r_ctOld)
            out_1l.append(dv_dyn)
            s += "{:7.3f},".format(t[i])
            s += "{:2d},".format(reset)
            s += "{:7.3f},".format(inp_)
            s += "{:7.3f},".format(out[i])
            s += "{:7.3f},".format(out_1[i])
            s += "{:7.3f},\n".format(out_1l[i])
        print(s)
        plt.figure()  # init 1
        plt.subplot(211)
        plt.title('StateSpace')
        plt.plot(t, out, color='red', linestyle='--', label='out_2nd_ord')
        plt.plot(t, out_1, color='blue', linestyle='-.', label='out_1st_ord')
        plt.plot(t, out_1l, color='green', linestyle=':', label='out_exp_lag')
        plt.legend(loc=1)
        plt.subplot(212)
        plt.plot(t, inp, color='black', linestyle='-', label='input')

        plt.show()


    if __name__ == '__main__':
        main()
