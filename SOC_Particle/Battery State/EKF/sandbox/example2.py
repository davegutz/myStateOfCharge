# Play with Kalman filters from Kalman-and-Bayesian-Filters-in-Python
# Press the green button in the gutter to run the script.
# install packages using 'python -m pip install numpy, matplotlib, scipy, pyfilter
# References:
#   [2] Roger Labbe. "Kalman and Bayesian Filters in Python"  -->kf_book
#   https://github.com/rlabbe/Kalman-and-Bayesian-Filters-in-Pythonimport numpy as np
# Dependencies:  python3.10, numpy, matplotlib, math, filterpy, ./kf_book
from numpy.random import randn
import numpy as np
import matplotlib.pyplot as plt
import math
from kf_book.nonlinear_plots import plot_gaussians
from filterpy.kalman import KalmanFilter
from filterpy.common import Q_discrete_white_noise
from math import sqrt


def univariate_filter(x0, P, R, Q):
    kf1 = KalmanFilter(dim_x=1, dim_z=1, dim_u=1)
    kf1.x = np.array([[x0]])
    kf1.P *= P
    kf1.H = np.array([[1.]])
    kf1.F = np.array([[1.]])
    kf1.B = np.array([[1.]])
    kf1.Q *= Q
    kf1.R *= R
    return kf1


def pos_vel_filter(x, P, R, Q=0., dt=1.0):
    """ Returns a KalmanFilter which implements a
    constant velocity model for a state [x dx].T
    """
    
    kf = KalmanFilter(dim_x=2, dim_z=1)
    kf.x = np.array([x[0], x[1]]) # location and velocity
    kf.F = np.array([[1., dt],
                     [0.,  1.]])  # state transition matrix
    kf.H = np.array([[1., 0]])    # Measurement function
    kf.R *= R                     # measurement uncertainty
    if np.isscalar(P):
        kf.P *= P                 # covariance matrix 
    else:
        kf.P[:] = P               # [:] makes deep copy
    if np.isscalar(Q):
        kf.Q = Q_discrete_white_noise(dim=2, dt=dt, var=Q)
    else:
        kf.Q[:] = Q
    return kf


def plot_1d_2d(xs, xs1d, xs2d):
    plt.plot(xs1d, label='1D Filter')
    plt.scatter(range(len(xs2d)), xs2d, c='r', alpha=0.7, label='2D Filter')
    plt.plot(xs, ls='--', color='k', lw=1, label='track')
    plt.title('State')
    plt.legend(loc=4)
    plt.show()

    
def compare_1D_2D(x0, P, R, Q, vel, u=None):
    # storage for filter output
    xs, xs1, xs2 = [], [], []

    # 1d KalmanFilter
    f1D = univariate_filter(x0, P, R, Q)

    #2D Kalman filter
    f2D = pos_vel_filter(x=(x0, vel), P=P, R=R, Q=0)
    if np.isscalar(u):
        u = [u]
    pos = 0 # true position
    for i in range(100):
        pos += vel
        xs.append(pos)

        # control input u - discussed below
        f1D.predict(u=u)
        f2D.predict()
        
        z = pos + randn()*sqrt(R) # measurement
        f1D.update(z)
        f2D.update(z)
        
        xs1.append(f1D.x[0])
        xs2.append(f2D.x[0])
    plt.figure()
    plot_1d_2d(xs, xs1, xs2)
    print('example2:  done')


if __name__ == '__main__':
    compare_1D_2D(x0=0, P=50., R=5., Q=.02, vel=1.)
