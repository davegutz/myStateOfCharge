# Play with Kalman filters from Kalman-and-Bayesian-Filters-in-Python
# Press the green button in the gutter to run the script.
import numpy as np
from numpy.random import randn
import matplotlib.pyplot as plt
import math
from collections import namedtuple
from kf_book.nonlinear_plots import plot_gaussians
gaussian = namedtuple('Gaussian', ['mean', 'var'])
gaussian.__repr__ = lambda s: '𝒩(μ={:.3f}, 𝜎²={:.3f})'.format(s[0], s[1])


# One-dimensional Kalman filter
def update(prior, measurement):
    x, P = prior  # mean and variance of prior
    z, R = measurement  # mean and variance of measurement

    y = z - x  # residual
    K = P / (P + R)  # Kalman gain

    x = x + K * y  # posterior
    P = (1 - K) * P  # posterior variance
    return gaussian(x, P)


def predict(posterior, movement):
    x, P = posterior  # mean and variance of posterior
    dx, Q = movement  # mean and variance of movement
    x = x + dx
    P = P + Q
    return gaussian(x, P)


def plot_kalman_filter(start_pos, sensor_noise, velocity, process_noise):
    plt.figure()
    # your code goes here
    print('PREDICT           UPDATE')
    print('   prior  var       z   var            filter  var')
    N = 100
    np.random.seed(303)
    zs, poss = [], []
    for i in range(N):
        z = math.sin(i / 3.) * 2 + randn() * sensor_noise
        zs.append(z)
    # Initialize
    # 1. Initialize state of the filter
    pos = gaussian(0., 1000.)  # mean and variance
    # 2.  Initialize our belief in the state
    process_model = gaussian(velocity, process_noise)
    for z in zs:
        # Predict
        # 1.  Use system behavior to predict state at next time step
        # 2.  Adjust belief to account for uncertainty in position
        prior = predict(pos, process_model)

        # Update
        # 1.  Get a measurement and associated belief about its accuracy
        # 2.  Compute residual between estimated state & measurement
        # 3.  Compute scaling factor based on whether the measurement or prediction is more accurate
        # 4.  Set the state between predicted and measured using scaling factor
        # 5.  Update belief in the state based on how certain we are in the measurement
        pos = update(prior, gaussian(z, sensor_noise))

        # Output
        poss.append(pos.mean)

    # plot
    plt.figure()
    plt.plot(zs, c='r', linestyle='dashed', label='measurement')
    plt.plot(poss, c='#004080', alpha=0.7, label='filter')
    plt.legend(loc=4)
    plt.show()


if __name__ == '__main__':
    plot_kalman_filter(start_pos=0, sensor_noise=5, velocity=1, process_noise=5)
