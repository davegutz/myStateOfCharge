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

A = np.array([[-1/Rct/Cct,       1/Rct/Cct,                          0],
              [1/Rct/(Cct+Cdif), -(Rct+Rdif)/Rct/Rdif/(Cct+Cdif),    0],
              [0,                1/Rdif/Cdif,                        -(Rdif+R0)/Rdif/R0/Cct]])
B = np.array([[1/Cct,    0],
              [0,        0],
              [0,        1/Cct/R0]])
C = np.array([[1,        0,      0],
              [0,        0,      1/R0]])
D = np.array([[0,        0],
              [0,        -1/R0]])

t = np.arange(0, 200+dt, dt)
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
        x = np.array([Vb, Vc, Vd]).T

    x_dot = A @ x + B @ u
    x = x_dot * dt + x
    y = C @ x + D @ u

    Vb_dot = x_dot[0]
    Vc_dot = x_dot[1]
    Vd_dot = x_dot[2]
    Vb = y[0]
    Vc = x[1]
    Vd = x[2]
    Ioc = y[1]
    I_Cct = (Vb_dot - Vc_dot)*Cct
    I_Cdif = (Vc_dot - Vd_dot)*Cdif
    I_Rct = (Vb - Vc) / Rct
    I_Rdif = (Vc - Vd) / Rdif

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

plt.figure()
plt.subplot(211)
plt.title('GP_battery.py')
plt.plot(t, Is, color='green', label='I')
plt.plot(t, I_Rcts, color='red', label='I_Rct')
plt.plot(t, I_Cdifs, color='green', label='I_Cdif')
plt.plot(t, I_Rdifs, color='orange', linestyle='--', label='I_Rdif')
plt.plot(t, Iocs, color='orange', label='Ioc')
plt.legend(loc=1)
plt.subplot(212)
plt.plot(t, Vbs, color='green', label='Vb')
plt.plot(t, Vocs, color='red', label='Voc')
plt.legend(loc=3)
plt.show()
