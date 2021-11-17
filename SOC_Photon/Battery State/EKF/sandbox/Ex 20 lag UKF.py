import matplotlib.pyplot as plt
import numpy as np
from filterpy.common import Q_discrete_white_noise
from numpy.random import randn
from filterpy.kalman import UnscentedKalmanFilter as UKF
from filterpy.kalman import MerweScaledSigmaPoints
import book_format

book_format.set_style()


class INSim:
    def __init__(self, pos, dt_):
        self.pos = pos
        self.dt = dt_

    def update(self, vel):
        """ Compute and returns next position. Incorporates
        random variation in velocity. """
        dx = vel*self.dt
        self.pos += dx
        return self.pos


class LagHardwareModel:
    # hardware model

    def __init__(self, pos, tau_, dt_, pos_std_):
        self.pos = pos
        self.tau = tau_
        self.dt = dt_
        self.pos_std = pos_std_

    def noisy_reading(self, pos_in):
        """ Return pos with
        simulated noise"""

        pos_past = self.pos
        vel = (pos_in - self.pos) / self.tau
        self.pos += vel*self.dt + randn()*self.pos_std
        return pos_past, vel

    def fx_calc(self, x, dt_, u, t_=None, z_=None, tau_=None):
        """ innovation function """
        out = np.empty_like(x)
        out[0] = x[1]*dt_ + x[0]
        # out[1] = x[1] + dt_/fx_calc.tau*(u - x[0])
        if tau_==None:
            out[1] = (u - out[0])/self.tau
        else:
            out[1] = (u - out[0])/tau_
        try:
            t_, z_
            print("t,z,x,dt,u = %6.3f, %7.3f, %7.3f, %7.3f, %7.3f," % (t_, z_, x[0], dt_, u))
        except:
            pass
        return out


def plot_lag(xs_, t_, plot_x=True, plot_vel=True):
    xs_ = np.asarray(xs_)
    if plot_x:
        plt.figure()
        plt.plot(t_, xs_[:, 0]/1000.)
        plt.xlabel('time(sec)')
        plt.ylabel('position()')
        plt.tight_layout()
    if plot_vel:
        plt.figure()
        plt.plot(t_, xs_[:, 1])
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

    def fx_calc_lin(self, x):
        """ state transition function for a constant velocity
        aircraft with state vector [x, velocity, altitude]'"""
        return self.F @ x


def h_lag(x):
    """ feedback function """
    return [x[0]]


# complete tracking ukf
tau_hardware = 0.159 # Hardware lag (0.159 for 1 Hz -3dB bandwidth)
tau_fx = 0.159  # Kalman lag estimate (0.159 for 1 Hz -3dB bandwidth)
dt = 0.1
pos_sense_std = 5    # Hardware sensor variation (1)

# UKF settings
r_std = .1  # Kalman sensor uncertainty (0.1)
q_std = 7  # Process uncertainty (7)

in_pos = 0
in_vel = 0
lag_pos = in_pos
lag_vel = in_vel

# Hardware simulation
in_lag20 = INSim(lag_pos, dt)
lag20_hardware = LagHardwareModel(lag_pos, tau_hardware, dt, pos_sense_std)

# Setup the UKF
# filter_lag = FilterLag(tau, dt, lag_pos, lag_vel)
points = MerweScaledSigmaPoints(n=2, alpha=.001, beta=2., kappa=1.)
kf = UKF(dim_x=2, dim_z=1, dt=dt, fx=lag20_hardware.fx_calc, hx=h_lag, points=points)
kf.Q = Q_discrete_white_noise(dim=2, dt=dt, var=q_std*q_std)
kf.R = r_std**2
kf.x = np.array([lag_pos, lag_vel])
kf.P = np.eye(2)*100

np.random.seed(200)

t = np.arange(0, 5+dt, dt)
n = len(t)

zs = []
refs = []
xs = []
vs = []
vhs = []
prior_x_est = []
prior_v_est = []
Ks = []
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
    z, vh = lag20_hardware.noisy_reading(ref)
    kf.predict(u=ref, tau_=tau_fx)
    kf.update(z)

    refs.append(ref)
    zs.append(z)
    vhs.append(vh)
    prior_x_est.append(kf.x_prior[0])
    prior_v_est.append(kf.x_prior[1])
    xs.append(kf.x[0])
    vs.append(kf.x[1])
    Ks.append(kf.K[0,0])

# print(zs)
# plt.plot(t, refs)
# plt.plot(t, zs)
# plt.show()
# UKF.batch_filter does not support keyword arguments fx_args, hx_args
# xs, covs = kf.batch_filter(zs)
# plot_lag(xs, t)
print(kf.x, 'log-likelihood', kf.log_likelihood, 'Kalman gain', kf.K)
plt.figure()
plt.subplot(221); plt.title('Ex 20 lag UKF.py')
plt.scatter(t, prior_x_est, color='green', label='Post X', marker='o')
plt.scatter(t, zs, color='black', label='Meas X', marker='.')
plt.plot(t, xs, color='green', label='Est X')
plt.plot(t, refs, color='blue', linestyle='--', label='Ref X')
plt.legend(loc=2)
# plt.show()
# plt.figure()
plt.subplot(222)
plt.scatter(t, vhs, color='black', label='Meas V', marker='.')
plt.plot(t, prior_v_est, color='green', label='Post V')
plt.legend(loc=3)
plt.subplot(223)
plt.plot(t, Ks, color='green', label='K')
plt.legend(loc=3)
plt.show()

