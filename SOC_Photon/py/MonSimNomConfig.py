# MonSimNomConfig:  nominal values of global parameters used by MonSim
# Copyright (C) 2022 Dave Gutz
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation;
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# See http://www.fsf.org/licensing/licenses/lgpl.txt for full license text.

""" Nominal values"""


# MonSim nominal inputs
dv_sense = 0.  # (0.-->0.1) ++++++++ flat curve
di_sense = 0.  # (0.-->0.5) ++++++++  i does not go to zero steady state
model_bat_cap = 100.  # (100.-->80) ++++++++++ dyn only provided reset soc periodically
r_0 = 0.0046  # (0.0046-->0.0092)  +++ dyn only provided reset soc periodically
tau_ct = 83  # (83-->100)  +++++++++++ dyn only provided reset soc periodically
r_ct = 0.0077  # (0.0077-->0.015)   ++++++++++  dyn only provided reset soc periodically
rsd = 70.  # (70.-->700)  ------- dyn only
tau_sd = 2.5e7  # (2.5e7-->2.5e6) ++++++ dyn only
v_std = 0.  # (0.01-->0) ------ noise
i_std = 0.  # (0.1-->0) ------ noise
hys_scale_sim = 1.0  # (1e-6<--  0.33-->10.) 1e-6 disables hysteresis
hys_scale_monitor = 1.0  # (1e-6<--  0.33-->10.) 1e-6 disables hysteresis
T_SAT = 10.  # Saturation time, sec 8/28/2022
T_DESAT = T_SAT * 2.  # De-saturation time, sec
