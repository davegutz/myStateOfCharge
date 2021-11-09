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
NOM_BATT_CAP = 100   # Nominal battery bank capacity, Ah (100).   Accounts for internal losses.  This is
                     # what gets delivered, e.g. Wshunt/NOM_SYS_VOLT.  Also varies 0.2-0.4C currents
                     # or 20-40 A for a 100 Ah battery
BATT_DVOC_DT = 0.001875 # Change of VOC with operating temperature in range 0 - 50 C (0.0075) V / deg C
BATT_V_SAT = 3.4625 # Normal battery cell saturation for SOC=99.7, V (3.4625 = 13.85v)
NOM_SYS_VOLT = 12   # Nominal system output, V, at which the reported amps are used (12)
nz_bb = 3
m_bb = 0.478
batt_num_cells = 4

lut_b = LookupTable()
lut_a = LookupTable()
lut_c = LookupTable()
lut_b.addAxis('T_degC', [0.,	    25.,    50.])
lut_a.addAxis('T_degC', [0.,	    25.,    50.])
lut_c.addAxis('T_degC', [0.,	    25.,    50.])
lut_b.setValueTable([-0.836,  -0.836, -0.836])    # b
lut_a.setValueTable([3.999,   4.046,  4.093])     # a
lut_c.setValueTable([-1.181,  -1.181, -1.181])    # c


def look(T_C):
    b = lut_b.lookup(T_degC=T_C)
    a = lut_a.lookup(T_degC=T_C)
    c = lut_c.lookup(T_degC=T_C)
    return b, a, c


d_bb = 0.707
n_bb = 0.4
cu_bb = NOM_BATT_CAP  # Assumed capacity, Ah
cs_bb = 102.          # Data fit to this capacity to avoid math 0, Ah
mxeps_bb = 1-1e-6     # Numerical maximum of coefficient model with scaled socs.
mneps_bb = 1e-6       # Numerical minimum of coefficient model without scaled socs.
mxepu_bb = 1-1e-6     # Numerical maximum of coefficient model with scaled socs.
mnepu_bb = 1 - (1-1e-6)*cs_bb/cu_bb  # Numerical minimum of coefficient model without scaled socs.
dvoc_dt = BATT_DVOC_DT * batt_num_cells


