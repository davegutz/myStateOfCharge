# Play with Kalman filters from Kalman-and-Bayesian-Filters-in-Python
# Press the green button in the gutter to run the script.
# install packages using 'python -m pip install numpy, matplotlib, scipy, pyfilter
# References:
#   [2] Roger Labbe. "Kalman and Bayesian Filters in Python"  -->kf_book
#   https://github.com/rlabbe/Kalman-and-Bayesian-Filters-in-Pythonimport numpy as np
# Dependencies:  python3.10,  ./pyDAGx


# Prepare for EKF with function call for battery
from pyDAGx.lookup_table import LookupTable


# Battery model LiFePO4 BattleBorn.xlsx and 'Generalized SOC-OCV Model Zhang etal.pdf'
# SOC-OCV curve fit './Battery State/BattleBorn Rev1.xls:Model Fit' using solver with min slope constraint
# >=0.02 V/soc.  m and n using Zhang values.   Had to scale soc because  actual capacity
# > NOM_BATT_CAP so equations error when soc<=0 to match data.
NOM_BATT_CAP = 100   # Nominal battery bank capacity, Ah (100).   Accounts for internal losses.  This is                     # what gets delivered, e.g. Wshunt/NOM_SYS_VOLT.  Also varies 0.2-0.4C currents
                     # or 20-40 A for a 100 Ah battery
BATT_DVOC_DT = 0.001875 # Change of VOC with operating temperature in range 0 - 50 C (0.0075) V / deg C
BATT_V_SAT = 3.4625 # Normal battery cell saturation for SOC=99.7, V (3.4625 = 13.85v)
NOM_SYS_VOLT = 12   # Nominal system output, V, at which the reported amps are used (12)
BATT_R1 = 0.00126
BATT_R2 = 0.00168
BATT_R2C2 = 100
cu_bb = NOM_BATT_CAP  # Assumed capacity, Ah
cs_bb = 102.          # Data fit to this capacity to avoid math 0, Ah
mxeps = 1-1e-6     # Numerical maximum of coefficient model with scaled socs.
mneps = 1e-6       # Numerical minimum of coefficient model without scaled socs.
mxepu = 1-1e-6     # Numerical maximum of coefficient model with scaled socs.
mnepu_bb = 1 - (1-mneps)*cs_bb/cu_bb  # Numerical minimum of coefficient model without scaled socs.
bb_t = [0., 25., 50.]
bb_b = [-0.836, -0.836, -0.836]
bb_a = [3.999, 4.046, 4.093]
bb_c = [-1.181, -1.181, -1.181]
d_bb = 0.707
n_bb = 0.4
m_bb = 0.478
batt_num_cells = int(NOM_SYS_VOLT/3)
bb_dvoc_dt = BATT_DVOC_DT * batt_num_cells
batt_vsat = float(batt_num_cells)*BATT_V_SAT
batt_vmax = 14.3/4*float(batt_num_cells)
batt_r1 = float(BATT_R1)
batt_r2 = float(BATT_R2)
batt_r2c2 = float(BATT_R2C2)
batt_c2 = batt_r2c2/batt_r2


# Battery
class Battery:
    def __init__(self, n_cells=1, dv=0., t_t=[25], t_b=[-.836], t_a=[4.046], t_c=[-1.181],
                 d=.707, n=0.4, m=0.478, dvoc_dt=0., r1=0.00126, r2=0.00168, nom_vsat=13.85,
                 cu=100., cs = 102.):
        self.n_cells_ = n_cells
        self.dv = dv
        self.lut_b = LookupTable()
        self.lut_a = LookupTable()
        self.lut_c = LookupTable()
        self.lut_b.addAxis('T_degC', t_t)
        self.lut_a.addAxis('T_degC', t_t)
        self.lut_c.addAxis('T_degC', t_t)
        self.lut_b.setValueTable(t_b)
        self.lut_a.setValueTable(t_a)
        self.lut_c.setValueTable(t_c)
        self.d = d
        self.n = n
        self.m = m
        self.dvoc_dt = dvoc_dt
        self.r1 = r1
        self.r2 = r2
        self.nom_vsat = nom_vsat
        self.cu = cu
        self.cs = cs

    def calculate(self, t_c=25., socu_frac=1., curr_in=0.):
        b, a, c = self.look(t_c)
        self.socu = socu_frac
        self.socs = 1.-(1.-self.socu)*self.cu/self.cs
        socs_lim = max(min(self.socs, mxeps), mneps)
        self.curr_in = curr_in
        
        self.v
        return self.v

    def look(self, T_C):
        b = self.lut_b.lookup(T_degC=T_C)
        a = self.lut_a.lookup(T_degC=T_C)
        c = self.lut_c.lookup(T_degC=T_C)
        return b, a, c


my_bb = Battery(n_cells=batt_num_cells, t_t=bb_t, t_b=bb_b, t_a=bb_a, t_c=bb_c, d=d_bb, n=n_bb, m=m_bb,
                dvoc_dt=bb_dvoc_dt, cu=cu_bb, cs=cs_bb)

for T in range(0, 50, 5):
    b, a, c = my_bb.look(T_C=T)
    print(T, b, a, c)

T = 25.
socu_frac = 1.
curr_in = 0.
print('my_bb.calculate = ', my_bb.calculate(t_c=T, socu_frac=socu_frac, curr_in=curr_in))



