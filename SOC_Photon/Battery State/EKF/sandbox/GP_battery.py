import matplotlib.pyplot as plt
import numpy as np
from filterpy.common import Q_discrete_white_noise
from numpy.random import randn
from filterpy.kalman import UnscentedKalmanFilter as UKF
from filterpy.kalman import MerweScaledSigmaPoints
import book_format
book_format.set_style()

# coefficient definition
Cct = 2300
Rct = 0.0016
Cdif = 10820
Rdif = 0.0077
R0 = 0.003
dt = 0.1

A = []

t = np.arange(0, 200+dt, dt)
n = len(t)

zs = []
for i in range(len(t)):
    if t[i] < 1:
        I = 0
        Voc = 13.5
        u = [I, Voc].T
    else:
        I = 30
        Voc = 13.5
        u = [I, Voc].T

    if i is 0:
        Vb = (R0 + Rdif + Rct) * I + Voc
        Vc = (R0 + Rdif) * I + Voc
        Vd = R0 * I + Voc
        x = [Vb, Vc, Vd].T

    x_dot = A @ x + B @ u

    zs.append(z)

plt.figure()
plt.subplot(221); plt.title('GP_battery.py')
plt.scatter(t, prior_x_est, color='green', label='Post X', marker='o')
plt.scatter(t, zs, color='black', label='Meas X', marker='.')
plt.plot(t, xs, color='green', label='Est X')
plt.plot(t, refs, color='blue', linestyle='--', label='Ref X')
plt.legend(loc=2)
plt.subplot(222)
plt.scatter(t, vhs, color='black', label='Meas V', marker='.')
plt.plot(t, prior_v_est, color='green', label='Post V')
plt.legend(loc=3)
plt.subplot(223)
plt.plot(t, Ks, color='green', label='K')
plt.legend(loc=3)
plt.show()

