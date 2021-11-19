import matplotlib.pyplot as plt
import numpy as np
import book_format
book_format.set_style()

# coefficient definition
Cct = 2300
Rct = 0.0016
Cdif = 10820
Rdif = 0.0077
R0 = 0.003
dt = 0.1

A = np.array([[-1/Rct/Cct,  0],
              [0,           -1/Rdif/Cdif]])
B = np.array([[1/Cct,   0],
              [1/Cdif,  0]])
C = np.array([[1,       1],
              [0,       0]])
D = np.array([[R0,      1],
              [1,       0]])
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
x = np.array([13.5, 13.5, 13.5]).T
for i in range(len(t)):
    if t[i] < 1:
        I = 0.0
        Voc = 13.5
        u = np.array([I, Voc]).T
    else:
        I = 30.0
        Voc = 13.5
        u = np.array([I, Voc]).T

    if i == 0:
        Vb = (R0 + Rdif + Rct) * I + Voc
        Vc = (R0 + Rdif) * I + Voc
        Vd = R0 * I + Voc
        Vbc = Vb - Vc
        Vcd = Vc - Vd
        Vdo = Vd - Voc
        x = np.array([Vbc, Vcd]).T

    x_dot = A @ x + B @ u
    x = x_dot * dt + x
    y = C @ x + D @ u

    Vbc_dot = x_dot[0]
    Vcd_dot = x_dot[1]
    Vbc = x[0]
    Vcd = x[1]
    Vd = Voc + I*R0
    Vc = Vd + Vcd
    Vb = Vc + Vbc
    Ioc = y[1]
    I_Cct = Vbc_dot*Cct
    I_Cdif = Vcd_dot*Cdif
    I_Rct = Vbc / Rct
    I_Rdif = Vcd / Rdif

    Is.append(I)
    Vocs.append(Voc)
    Vbs.append(Vb)
    Vcs.append(Vc)
    Vds.append(Vd)
    Iocs.append(Ioc)
    I_Ccts.append(I_Cct)
    I_Cdifs.append(I_Cdif)
    I_Rcts.append(I_Rct)
    I_Rdifs.append(I_Rdif)
    Vbc_dots.append(Vbc_dot)
    Vcd_dots.append(Vcd_dot)

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
