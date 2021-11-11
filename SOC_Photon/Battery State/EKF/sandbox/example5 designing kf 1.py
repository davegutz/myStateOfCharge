# straight from 08-Designing-Kalman-Filters
import numpy as np
from numpy.random import randn
import matplotlib.pyplot as plt
from kf_book.book_plots import plot_measurements, plot_filter
from filterpy.stats import plot_covariance_ellipse
from filterpy.kalman import KalmanFilter
from scipy.linalg import block_diag
from filterpy.common import Q_discrete_white_noise


class PosSensor(object):
    def __init__(self, pos=(0, 0), vel=(0, 0), noise_std=1.):
        self.vel = vel
        self.noise_std = noise_std
        self.pos = [pos[0], pos[1]]

    def read(self):
        self.pos[0] += self.vel[0]
        self.pos[1] += self.vel[1]
        #np.random.seed(5)
        return [self.pos[0] + randn() * self.noise_std,
                self.pos[1] + randn() * self.noise_std]


R_std = 0.35
Q_std = 0.04


def tracker1():
    tracker = KalmanFilter(dim_x=4, dim_z=2)
    dt = 1.0  # time step

    tracker.F = np.array([[1, dt, 0, 0],
                          [0, 1, 0, 0],
                          [0, 0, 1, dt],
                          [0, 0, 0, 1]])
    tracker.u = 0.
    tracker.H = np.array([[1 / 0.3048, 0, 0, 0],
                          [0, 0, 1 / 0.3048, 0]])

    tracker.R = np.eye(2) * R_std ** 2
    q = Q_discrete_white_noise(dim=2, dt=dt, var=Q_std ** 2)
    tracker.Q = block_diag(q, q)
    tracker.x = np.array([[0, 0, 0, 0]]).T
    tracker.P = np.eye(4) * 500.
    return tracker


# simulate robot movement
N = 30
sensor = PosSensor((0, 0), (2, .2), noise_std=R_std)

zs = np.array([sensor.read() for _ in range(N)])

# run filter
robot_tracker = tracker1()
mu, cov, _, _ = robot_tracker.batch_filter(zs)

for x, P in zip(mu, cov):
    # covariance of x and y
    cov = np.array([[P[0, 0], P[2, 0]],
                    [P[0, 2], P[2, 2]]])
    mean = (x[0, 0], x[2, 0])
    plot_covariance_ellipse(mean, cov=cov, fc='g', std=3, alpha=0.5)

# plot results
np.set_printoptions(precision=3)
zs *= .3048  # convert to meters
plt.clf()
plot_filter(mu[:, 0], mu[:, 2])
plot_measurements(zs[:, 0], zs[:, 1])
plt.ylim(-3, 5)
plt.xlim(0, 20)
plt.grid()
plt.legend(loc=2)
print(zs)
plt.show()
