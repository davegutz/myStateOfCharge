# Filter classes
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

__author__ = 'Dave Gutz <davegutz@alum.mit.edu>'
__version__ = '$Revision: 1.1 $'
__date__ = '$Date: 2022/06/10 13:15:02 $'


# 1-pole filters
class DiscreteFilter:
    # Base class for 1-pole filters

    def __init__(self, dt, tau, min_, max_):
        self.max = max_
        self.min = min_
        self.rate = 0.
        self.dt = dt
        self.tau = tau

    def calculate_(self, in_, reset):
        if reset:
            self.rate = 0.
        return self.rate

    def calculate(self, in_, reset, dt):
        raise NotImplementedError

    def assign_coeff(self, tau):
        raise NotImplementedError

    def rate_state(self, in_):
        raise NotImplementedError

    def rate_state_calc(self, in_, dt, reset):
        raise NotImplementedError

    def state(self):
        raise NotImplementedError


# 1-pole filters
class DiscreteFilter2:
    # Base class for 2-pole filters

    def __init__(self, dt, omega_n, zeta, min_, max_):
        self.max = max_
        self.min = min_
        self.omega_n = omega_n
        self.dt = dt
        self.zeta = zeta
        self.rate = 0.

    def calculate_(self, in_, reset):
        if reset:
            self.rate = 0.
        return self.rate

    def calculate(self, in_, reset, dt):
        raise NotImplementedError

    def assign_coeff(self, tau):
        raise NotImplementedError

    def rate_state(self, in_, reset):
        raise NotImplementedError

    def rate_state_calc(self, in_, dt, reset):
        raise NotImplementedError

    def state(self):
        raise NotImplementedError


class DiscreteIntegrator:
    """Generic integrator"""

    def __init__(self, dt, min_, max_, a, b, c):
        self.a = a
        self.b = b
        self.c = c
        self.lim = False
        self.max = max_
        self.min = min_
        self.state = 0.
        self.rate_state = 0.
        self.dt = dt

    def new_state(self, new_state):
        self.state = max(min(new_state, self.max), self.min)
        self.rate_state = 0.

    def calculate_(self, in_, reset, init_value):
        if reset:
            self.state = init_value
            self.rate_state = 0.
        else:
            self.state += (self.a*in_ + self.b*self.rate_state) * self.dt / self.c
        if self.state < self.min:
            self.state = min
            self.lim = True
            self.rate_state = 0.
        elif self.state > self.max:
            self.state = max
            self.lim = True
            self.rate_state = 0.
        else:
            self.lim = False
            self.rate_state = in_
        return self.state

    def calculate(self, in_, dt, reset, init_value):
        self.dt = dt
        if reset:
            self.state = init_value
            self.rate_state = 0.
        else:
            self.state += (self.a*in_ + self.b*self.rate_state) * self.dt / self.c
        if self.state < self.min:
            self.state = min
            self.lim = True
            self.rate_state = 0.
        elif self.state > self.max:
            self.state = max
            self.lim = True
            self.rate_state = 0.
        else:
            self.lim = False
            self.rate_state = in_
        return self.state


class AB2Integrator(DiscreteIntegrator):
    """AB2 Integrator"""

    def __init__(self, dt, min_, max_):
        DiscreteIntegrator.__init__(self, dt, min_, max_, 3., -1., 2.)


class TustinIntegrator(DiscreteIntegrator):
    """AB2 Integrator"""

    def __init__(self, dt, min_, max_):
        DiscreteIntegrator.__init__(self, dt, min_, max_, 1., 1., 2.)


class General2Pole(DiscreteFilter2):
    # General 2-Pole filter variable update rate and limits, poor aliasing characteristics

    def __init__(self, dt, omega_n, zeta, min_, max_):
        DiscreteFilter2.__init__(self, dt, omega_n, zeta, min_, max_)
        self.a = 2. * self.zeta * self.omega_n
        self.b = self.omega_n * self.omega_n
        self.assign_coeff(dt)
        self.AB2 = AB2Integrator(dt, min, max)
        self.Tustin = TustinIntegrator(dt, min, max)

    def assign_coeff(self, dt):
        self.dt = dt

    def calculate_(self, in_, reset):
        self.rate_state(in_, reset)

    def rate_state(self, in_, reset):
        if reset:
            accel = 0.
        else:
            accel = self.b*(in_ - self.Tustin.state) - self.a*self.AB2.state
        self.Tustin.calculate(self.AB2.calculate(accel, self.dt, reset, 0), self.dt, reset, in_)
        if self.Tustin.lim:
            self.AB2.new_state(0)

    def rate_state_calc(self, in_, dt, reset):
        self.assign_coeff(dt)
        self.rate_state(in_, reset)


class LagTustin(DiscreteFilter):
    # Tustin lag calculator, non-pre-warped

    def __init__(self, dt, tau, min_, max_):
        DiscreteFilter.__init__(self, dt, tau, min_, max_)
        self.a = 0.
        self.b = 0.
        self.rate = 0.
        self.state = 0.
        self.assign_coeff(tau)

    def assign_coeff(self, tau):
        self.tau = tau
        self.a = 2.0 / (2.0 * self.tau + self.dt)
        self.b = (2.0 * self.tau - self.dt) / (2.0 * self.tau + self.dt)

    def calc_state_(self, in_):
        self.rate = max(min(self.a * (in_ - self.state), self.max), self.min)
        self.state = max(min(in_ * (1.0 - self.b) + self.state * self.b, self.max), self.min)

    def calc_state(self, in_, dt):
        self.dt = dt
        self.assign_coeff(self.tau)
        self.calc_state_(in_)

    def calculate(self, in_, reset, dt):
        if reset:
            self.state = in_
        self.calc_state(in_, dt)
        return self.state

    def state(self):
        return self.state

