# BatteryEKF - general purpose battery class for embedded EKF
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

"""Coulomb Counting.  1 Ampere is 1 Coulombs/second charge rate of change.   Herein are various methods to keep track
of the totals and standardize the calculations."""

# Constants
import numpy as np
from pyDAGx import myTables
from Chemistry_BMS import Chemistry, BMS

dqdt = 0.01  # Change of charge with temperature, fraction/deg C.  From literature.  0.01 is commonly used
coul_eff = 0.9985  # Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs


class Coulombs:
    """Coulomb Counting"""

    def __init__(self, q_cap_rated, q_cap_rated_scaled, t_rated, t_rlim=0.017, tweak_test=False,
                 coul_eff_= 0.9985, mod_code=0):
        self.q_cap_rated = q_cap_rated
        self.q_cap_rated_scaled = q_cap_rated_scaled
        self.t_rated = t_rated
        self.t_rlim = t_rlim
        self.delta_q = 0.
        self.q = 0.
        self.q_capacity = 0.
        self.soc_min = 0.
        self.soc = 1.
        self.resetting = True
        self.q_min = 0.
        self.temp_lim = 0.
        self.t_last = 0.
        self.sat = True
        self.chm = mod_code
        self.coul_eff = coul_eff_
        self.tweak_test = tweak_test
        self.reset = False
        self.chemistry = Chemistry(mod_code=mod_code)

    def __str__(self, prefix=''):
        """Returns representation of the object"""
        s = prefix + "Coulombs:\n"
        s += "  q_cap_rated = {:9.1f}    // Rated capacity at t_rated_, saved for future scaling, C\n"\
            .format(self.q_cap_rated)
        s += "  q_cap_rated_scaled = {:9.1f} // Applied rated capacity at t_rated_, after scaling, C\n"\
            .format(self.q_cap_rated_scaled)
        s += "  q_capacity = {:9.1f}     // Saturation charge at temperature, C\n".format(self. q_capacity)
        s += "  q =          {:9.1f}     // Present charge available to use, C\n".format(self. q)
        s += "  delta_q      {:9.1f}     // Charge since saturated, C\n".format(self. delta_q)
        s += "  soc =        {:7.3f}       // Fraction of saturation charge (q_capacity_) available (0-1)  soc)\n"\
            .format(self.soc)
        s += "  sat =          {:1.0f}          // Indication from caller that battery is saturated, T=saturated\n"\
            .format(self.sat)
        s += "  t_rated =    {:5.1f}         // Rated temperature, deg C\n".format(self. t_rated)
        s += "  t_last =     {:5.1f}         // Last battery temperature for rate limit memory, deg C\n"\
            .format(self.t_last)
        s += "  t_rlim =     {:7.3f}       // Tbatt rate limit, deg C / s\n".format(self. t_rlim)
        s += "  resetting =     {:d}          // Flag to test coulomb counters, T = external reset of counter\n"\
            .format(self.resetting)
        s += "  soc_min =    {:7.3f}       // Lowest soc for power delivery.   Arises with temp < 20 C\n"\
            .format(self.soc_min)
        s += "  tweak_test =    {:d}          // Driving signal injection completely using software inj_soft_bias\n"\
            .format(self.tweak_test)
        s += "  coul_eff =   {:8.4f}      // Coulombic efficiency- fraction of charging turned into usable Coulombs\n".\
            format(self.coul_eff)
        return s

    def apply_cap_scale(self, scale):
        """ Scale size of battery and adjust as needed to preserve delta_q.  Tb unchanged.
        Goal is to scale battery and see no change in delta_q on screen of
        test comparisons.   The rationale for this is that the battery is frequently saturated which
        resets all the model parameters.   This happens daily.   Then both the model and the battery
        are discharged by the same current so the delta_q will be the same."""
        self.q_cap_rated_scaled = scale * self.q_cap_rated
        self.q_capacity = self.calculate_capacity(self.t_last)
        self.q = self.delta_q + self.q_capacity  # preserve self.delta_q, deficit since last saturation(like real life)
        self.soc = self.q / self.q_capacity
        self.resetting = True  # momentarily turn off saturation check

    def apply_delta_q(self, delta_q, temp_c):
        """Memory set, adjust bookkeeping as needed.  delta_q, capacity, temp preserved"""
        self.delta_q = delta_q
        self.q_capacity = self.calculate_capacity(temp_c)
        self.q = self.delta_q + self.q_capacity
        self.soc = self.q / self.q_capacity
        self.resetting = True  # momentarily turn off saturation check

    def apply_delta_q_t(self, delta_q, temp_c):
        self.delta_q = delta_q
        self.q_capacity = self.calculate_capacity(temp_c)
        self.q = self.q_capacity + self.delta_q
        self.soc = self.q / self.q_capacity
        self.resetting = True

    def apply_soc(self, soc, temp_c):
        """Memory set, adjust bookkeeping as needed.  delta_q preserved"""
        self.soc = soc
        self.q_capacity = self.calculate_capacity(temp_c)
        self.q = soc*self.q_capacity
        self.delta_q = self.q - self.q_capacity
        self.resetting = True  # momentarily turn off saturation check

    def calculate_capacity(self, temp_c):
        """Capacity"""
        try:
            res = self.q_cap_rated_scaled * (1. + dqdt * (temp_c - self.t_rated))
        except:
            res = 1
        return res
        # return self.q_cap_rated_scaled * (1. + dqdt * (temp_c - self.t_rated))

    def count_coulombs(self, chem, dt, reset, temp_c, charge_curr, sat, soc_init=None):
        """Count coulombs based on true=actual capacity
        Inputs:
            dt              Integration step, s
            temp_c          Battery temperature, deg C
            charge_curr     Charge, A
            sat             Indicator that battery is saturated (VOC>threshold(temp)), T/F
            t_last          Past value of battery temperature used for rate limit memory, deg C
            coul_eff        Coulombic efficiency - the fraction of charging input that gets turned into usable Coulombs
        """
        if self.chm != chem:
            self.chemistry.assign_all_mod(chem)
            self.chm = chem
        self.reset = reset
        d_delta_q = charge_curr * dt
        if charge_curr > 0. and not self.tweak_test:
            d_delta_q *= self.coul_eff
        self.sat = sat

        # Rate limit temperature
        self.temp_lim = max(min(temp_c, self.t_last + self.t_rlim*dt), self.t_last - self.t_rlim*dt)
        if reset:
            self.temp_lim = temp_c
            self.t_last = temp_c

        # Saturation.   Goal is to set q_capacity and hold it so remembers last saturation status.
        if sat:
            if d_delta_q > 0:
                d_delta_q = 0.
                if ~self.resetting:
                    self.delta_q = 0.
        # elif reset:
        #     self.delta_q = 0.
        self.resetting = False  # one pass flag.  Saturation debounce should reset next pass

        # Integration
        self.q_capacity = self.calculate_capacity(self.temp_lim)
        self.delta_q = max(min(self.delta_q + d_delta_q - dqdt*self.q_capacity*(self.temp_lim-self.t_last),
                               0.0), -self.q_capacity*1.5)
        self.q = self.q_capacity + self.delta_q

        # Normalize
        self.soc = self.q / self.q_capacity
        self.soc_min = self.chemistry.lut_min_soc.interp(self.temp_lim)
        self.q_min = self.soc_min * self.q_capacity

        # Save and return
        self.t_last = self.temp_lim
        return self.soc

    def load(self, delta_q, t_last):
        """Load states from retained memory"""
        self.delta_q = delta_q
        self.t_last = t_last
