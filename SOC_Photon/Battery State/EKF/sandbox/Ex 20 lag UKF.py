import matplotlib.pyplot as plt
import numpy as np
from filterpy.common import Q_discrete_white_noise
from numpy.random import randn
from filterpy.kalman import UnscentedKalmanFilter as UKF
from filterpy.kalman import unscented_transform, MerweScaledSigmaPoints

import book_format
book_format.set_style()


class INSim:
    def __init__(self, pos, dt, vel_std):
        self.pos = pos
        self.dt = dt
        self.vel_std = vel_std

    def update(self, vel):
        """ Compute and returns next position. Incorporates
        random variation in velocity. """

        dx = (vel + randn()*self.vel_std)*self.dt
        self.pos += dx
        return self.pos


class LagHardwareModel:
    # hardware model

    def __init__(self, pos, tau, dt, pos_std):
        self.pos = pos
        self.tau = tau
        self.dt = dt
        self.pos_std = pos_std

    def noisy_reading(self, pos_in):
        """ Return pos with
        simulated noise"""

        vel = (pos_in - self.pos) / self.tau
        self.pos += vel*self.dt + randn()*self.pos_std
        return self.pos


def plot_lag(xs, t, plot_x=True, plot_vel=True):
    xs = np.asarray(xs)
    if plot_x:
        plt.figure()
        plt.plot(t, xs[:, 0]/1000.)
        plt.xlabel('time(sec)')
        plt.ylabel('position()')
        plt.tight_layout()
    if plot_vel:
        plt.figure()
        plt.plot(t, xs[:, 1])
        plt.xlabel('time(sec)')
        plt.ylabel('velocity')
        plt.tight_layout()
    plt.show()


class FilterLag:

    def __init__(self, tau_, dt_, pos, vel):
        self.tau = tau_
        self.dt = dt_
        self.pos = pos
        self.vel = vel
        self.F = np.array([[1, self.dt],
                      [-self.dt/self.tau, 1]], dtype=float)

    def f_calc_lin(self, x):
        """ state transition function for a constant velocity
        aircraft with state vector [x, velocity, altitude]'"""
        return self.F @ x


def f_calc(x, dt):
    """ innovation function """
    print(x)
    out = np.empty_like(x)
    out[0] = x[1]*dt + x[0]
    out[1] = x[1] - dt/f_calc.tau*x[0]
    return out


def h_lag(x):
    """ feedback function """
    return [x[0]]


# complete tracking ukf
tau = 0.159
f_calc.tau = tau
dt = 0.1
pos_std = 1  # meas noise
vel_std = 0.1

in_pos = 0
in_vel = 0
lag_pos = in_pos
lag_vel = in_vel

# Setup the UKF
filter_lag = FilterLag(tau, dt, lag_pos, lag_vel)
points = MerweScaledSigmaPoints(n=2, alpha=.1, beta=2., kappa=0.)
kf = UKF(2, 1, dt, fx=f_calc, hx=h_lag, points=points)
kf.Q = Q_discrete_white_noise(dim=2, dt=dt, var=0.1)
kf.R = pos_std**2
kf.x = np.array([lag_pos, lag_vel])
kf.P = np.eye(2)*10

in_lag20 = INSim(lag_pos, dt, 0)
lag20 = LagHardwareModel(lag_pos, tau, dt, pos_std)

np.random.seed(200)

t = np.arange(0, 5, dt)
n = len(t)

zs = []; refs = []
for i in range(len(t)):
    if t[i] < 1:
        v = 0
    elif t[i] < 1.8:
        v = 100
    elif t[i] < 3.0:
        v = 0
    elif t[i] < 3.8:
        v = -100
    else:
        v = 0
    ref = in_lag20.update(v)
    refs.append(ref)
    z = lag20.noisy_reading(ref)
    zs.append(z)


# print(zs)
# plt.plot(t, refs)
# plt.plot(t, zs)
# plt.show()
xs, covs = kf.batch_filter(zs)
# plot_lag(xs, t)
plt.figure()
plt.plot(t, refs)
plt.plot(t, zs)
plt.plot(t, xs)
plt.show()

