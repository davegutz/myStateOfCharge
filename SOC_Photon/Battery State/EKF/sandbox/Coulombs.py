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
cap_droop_c = 20.
dqdt = 0.01


class Coulombs:
    """Coulomb Counting"""

    def __init__(self, q_cap_rated_=0., q_cap_rated_scaled_=0., t_rated_=25., t_rlim_=0.017):
        self.q_cap_rated = q_cap_rated_
        self.q_cap_rated_scaled = q_cap_rated_scaled_
        self.t_rated = t_rated_
        self.t_rlim = t_rlim_
        self.delta_q = 0.
        self.q = 0.
        self.q_capacity = 0.
        self.soc_min = 0.
        self.soc = 1.
        self.SOC = 100.
        self.resetting = True
        self.q_min = 0.
        self.t_last = 0.
        self.sat = True

    def apply_cap_scale(self, scale_=1.):
        """ Scale size of battery and adjust as needed to preserve delta_q.  Tb unchanged.
        Goal is to scale battery and see no change in delta_q on screen of
        test comparisons.   The rationale for this is that the battery is frequently saturated which
        resets all the model parameters.   This happens daily.   Then both the model and the battery
        are discharged by the same current so the delta_q will be the same."""
        self.q_cap_rated_scaled = scale_ * self.q_cap_rated
        self.q_capacity = self.calculate_capacity(self.t_last)
        self.q = self.delta_q + self.q_capacity  # preserve self.delta_q, deficit since last saturation(like real life)
        self.soc = self.q / self.q_capacity
        self.SOC = self.q / self.q_cap_rated_scaled * 100.
        self.resetting = True  # momentarily turn off saturation check

    def apply_delta_q(self, delta_q_=0.):
        """Memory set, adjust book-keeping as needed.  delta_q, capacity, temp preserved"""
        self.delta_q = delta_q_
        self.q = self.delta_q + self.q_capacity
        self.soc = self.q / self.q_capacity
        self.SOC = self.q / self.q_cap_rated_scaled * 100.
        self.resetting = True  # momentarily turn off saturation check

    def apply_delta_q_t(self, delta_q_=0., temp_c_=25.):
        self.delta_q = delta_q_
        self.q_capacity = self.calculate_capacity(temp_c_)
        self.q = self.q_capacity + self.delta_q
        self.soc = self.q / self.q_capacity
        self.SOC = self.q / self.q_cap_rated_scaled * 100.
        self.resetting = True

    def apply_soc(self, soc_=1.):
        """Memory set, adjust book-keeping as needed.  delta_q preserved"""
        self.soc = soc_
        self.q = soc_*self.q_capacity
        self.delta_q = self.q - self.q_capacity
        self.SOC = self.q / self.q_cap_rated_scaled * 100.
        self.resetting = True  # momentarily turn off saturation check

    def apply_SOC(self, soc_=100.):
        """Memory set, adjust book-keeping as needed.  delta_q preserved"""
        self.SOC = soc_
        self.q = self.SOC / 100. * self.q_cap_rated_scaled
        self.delta_q = self.q - self.q_capacity
        self.soc = self.q / self.q_capacity
        self.resetting = True  # momentarily turn off saturation check

    def calculate_capacity(self, temp_c_=25.):
        """Capacity"""
        return self.q_cap_rated_scaled * (1. - dqdt * (temp_c_ - self.t_rated))

    def count_coulombs(self, dt_=0., reset_=True, temp_c_=25., charge_curr_=0., sat_=True, t_last_=0.):
        """Count coulombs based on true=actual capacity
        Inputs:
            dt              Integration step, s
            temp_c          Battery temperature, deg C
            charge_curr     Charge, A
            sat             Indicator that battery is saturated (VOC>threshold(temp)), T/F
            t_last          Past value of battery temperature used for rate limit memory, deg C
        """
        d_delta_q = charge_curr_ * dt_
        self.sat = sat_

        # Rate limit temperature
        temp_lim = max(min(temp_c_, t_last_ + self.t_rlim*dt_), t_last_ - self.t_rlim*dt_)
        if reset_:
            temp_lim = temp_c_

        # Saturation.   Goal is to set q_capacity and hold it so remember last saturation status.
        if sat_:
            if d_delta_q > 0:
                d_delta_q = 0.
                if ~self.resetting:
                    self.delta_q = 0.
        elif reset_:
            self.delta_q = 0.
        self.resetting = False  # one pass flag.  Saturation debounce should reset next pass

        # Integration
        self.q_capacity = self.calculate_capacity(temp_lim)
        self.delta_q = max(min(self.delta_q + d_delta_q - dqdt*self.q_capacity*(temp_lim-self.t_last),
                               0.0), -self.q_capacity)
        self.q = self.q_capacity + self.delta_q

        # Normalize
        self.soc = self.q / self.q_capacity
        self.soc_min = max((cap_droop_c - temp_lim)*dqdt, 0.)
        self.q_min = self.soc_min * self.q_capacity
        self.SOC = self.q / self.q_cap_rated_scaled * 100.

        # Save and return
        self.t_last = temp_lim
        return self.soc

    def load(self, delta_q_, t_last_):
        """Load states from retained memory"""
        self.delta_q = delta_q_
        self.t_last = t_last_

    def update(self):
        """Update states to be saved in retained memory"""
        return self.delta_q, self.t_last
