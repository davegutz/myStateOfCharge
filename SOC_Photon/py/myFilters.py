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


class RateLimit:
    def __init__(self):
        self.rate_min = 0.
        self.rate_max = 0.
        self.y = 0.
        self.y_past = 0.
        self.rate = 0.
        self.dt = 0.

    def update(self, x, reset, dt, min_, max_):
        self.rate_min = min_
        self.rate_max = max_
        self.dt = dt
        if reset:
            self.y = x
            self.y_past = x
        else:
            self.y = max(min(x, self.y_past + self.rate_max*self.dt), self.y_past + self.rate_min*self.dt)
        self.rate = (self.y - self.y_past) / self.dt
        self.y_past = self.y
        return self.y


class SlidingDeadband:
    def __init__(self, hdb):
        self.z = 0.
        self.hdb = hdb

    def update(self, in_):
        self.z = max(min( self.z, in_+self.hdb), in_-self.hdb)
        return self.z

    def update_res(self, in_, reset):
        if reset:
            self.z = in_
            return self.z
        out = self.update(in_)
        return out


# 1-pole filters
class DiscreteFilter:
    # Base class for 1-pole filters

    def __init__(self, dt, tau, min_, max_):
        self.dt = dt
        self.tau = tau
        self.min = min_
        self.max = max_
        self.rate = 0.

    def __str__(self, prefix=''):
        s = prefix + "DiscreteFilter:\n"
        s += "  dt       =    {:7.3f}  // Update time, s\n".format(self.dt)
        s += "  tau      =    {:7.3f}  // Natural frequency, r/s\n".format(self.tau)
        s += "  min      =    {:7.3f}  // Min output\n".format(self.min)
        s += "  max      =    {:7.3f}  // Max output\n".format(self.max)
        s += "  rate     =    {:7.3f}  // Calculated rate\n".format(self.rate)
        return s

    def calculate_(self, reset):
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
        self.dt = dt
        self.omega_n = omega_n
        self.zeta = zeta
        self.min = min_
        self.max = max_
        self.rate = 0.

    def __str__(self, prefix=''):
        s = prefix + "DiscreteFilter2:\n"
        s += "  dt       =    {:7.3f}  // Update time, s\n".format(self.dt)
        s += "  omega_n  =    {:7.3f}  // Natural frequency, r/s\n".format(self.omega_n)
        s += "  zeta     =    {:7.3f}  // Damping factor\n".format(self.zeta)
        s += "  min      =    {:7.3f}  // Min output\n".format(self.min)
        s += "  max      =    {:7.3f}  // Max output\n".format(self.max)
        s += "  rate     =    {:7.3f}  // Calculated rate\n".format(self.rate)
        return s

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
        self.dt = dt
        self.a = a
        self.b = b
        self.c = c
        self.lim = False
        self.max = max_
        self.min = min_
        self.state = 0.
        self.rate_state = 0.

    def __str__(self, prefix=''):
        s = prefix + "DiscreteIntegrator:\n"
        s += "  dt       =    {:7.3f}  // Update time, s\n".format(self.dt)
        s += "  a        =    {:7.3f}  // Coefficient\n".format(self.a)
        s += "  b        =    {:7.3f}  // Coefficient\n".format(self.b)
        s += "  c        =    {:7.3f}  // Coefficient\n".format(self.c)
        s += "  min      =    {:7.3f}  // Min output\n".format(self.min)
        s += "  max      =    {:7.3f}  // Max output\n".format(self.max)
        s += "  state    =    {:7.3f}  // State\n".format(self.state)
        s += "  rate_state=   {:7.3f}  // Rate state\n".format(self.rate_state)
        return s

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
            self.state = self.min
            self.lim = True
            self.rate_state = 0.
        elif self.state > self.max:
            self.state = self.max
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
            self.state = self.min
            self.lim = True
            self.rate_state = 0.
        elif self.state > self.max:
            self.state = self.max
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

    def __str__(self, prefix=''):
        s = prefix + "AB2Integrator:\n"
        s += "\n  "
        s += DiscreteIntegrator.__str__(self, prefix + 'AB2Integrator:')
        return s


class TustinIntegrator(DiscreteIntegrator):
    """AB2 Integrator"""

    def __init__(self, dt, min_, max_):
        DiscreteIntegrator.__init__(self, dt, min_, max_, 1., 1., 2.)

    def __str__(self, prefix=''):
        s = prefix + "TustinIntegrator:\n"
        s += "\n  "
        s += DiscreteIntegrator.__str__(self, prefix + 'TustinIntegrator:')
        return s


class General2Pole(DiscreteFilter2):
    # General 2-Pole filter variable update rate and limits, poor aliasing characteristics

    def __init__(self, dt, omega_n, zeta, min_, max_):
        DiscreteFilter2.__init__(self, dt, omega_n, zeta, min_, max_)
        self.a = 2. * self.zeta * self.omega_n
        self.b = self.omega_n * self.omega_n
        self.assign_coeff(dt)
        self.AB2 = AB2Integrator(dt, min_, max_)
        self.Tustin = TustinIntegrator(dt, min_, max_)
        self.in_ = 0.
        self.out_ = 0.
        self.accel = 0.
        self.saved = Saved2()

    def __str__(self, prefix=''):
        s = prefix + "General2Pole:\n"
        s += DiscreteFilter2.__str__(self, prefix + 'General2Pole:')
        s += "\n  "
        s += self.AB2.__str__(prefix + 'General2Pole:')
        s += "\n  "
        s += self.Tustin.__str__(prefix + 'General2Pole:')
        s += "  a        =    {:7.3f}  // Discrete coefficient\n".format(self.a)
        s += "  b        =    {:7.3f}  // Discrete coefficient\n".format(self.b)
        s += "  accel    =    {:7.3f}  // Input\n".format(self.accel)
        s += "  in_      =    {:7.3f}  // Input\n".format(self.in_)
        s += "  out_     =    {:7.3f}  // Output\n".format(self.out_)
        return s

    def assign_coeff(self, dt):
        self.dt = dt

    def calculate_(self, in_, reset):
        self.in_ = in_
        self.rate_state(self.in_, reset)

    def calculate(self, in_, reset, dt):
        self.in_ = in_
        self.assign_coeff(dt)
        self.rate_state_calc(self.in_, dt, reset)
        self.out_ = self.Tustin.state
        return self.out_

    def rate_state(self, in_, reset):
        if reset:
            self.accel = 0.
        else:
            self.accel = self.b * (in_ - self.Tustin.state) - self.a * self.AB2.state
        self.Tustin.calculate(self.AB2.calculate(self.accel, self.dt, reset, 0), self.dt, reset, in_)
        if self.Tustin.lim:
            self.AB2.new_state(0)

    def rate_state_calc(self, in_, dt, reset):
        self.assign_coeff(dt)
        self.rate_state(in_, reset)

    def save(self, time):
        self.saved.time.append(time)
        self.saved.in_.append(self.in_)
        self.saved.out_.append(self.out_)
        self.saved.accel.append(self.accel)


class LagTustin(DiscreteFilter):
    # Tustin lag calculator, non-pre-warped

    def __init__(self, dt, tau, min_, max_):
        DiscreteFilter.__init__(self, dt, tau, min_, max_)
        self.a = 0.
        self.b = 0.
        self.rate = 0.
        self.state = 0.
        self.assign_coeff(tau)
        self.saved = Saved1()
        self.in_ = 0.
        self.out_ = 0.

    def __str__(self, prefix=''):
        s = prefix + "LagTustin:"
        s += "\n  "
        s += DiscreteFilter.__str__(self, prefix + 'LagTustin:')
        s += "  a        =    {:7.3f}  // Discrete coefficient\n".format(self.a)
        s += "  b        =    {:7.3f}  // Discrete coefficient\n".format(self.b)
        s += "  in_      =    {:7.3f}  // Input\n".format(self.in_)
        s += "  out_     =    {:7.3f}  // Output\n".format(self.out_)
        return s

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
        self.in_ = in_
        if reset:
            self.state = self.in_
        self.calc_state(self.in_, dt)
        self.out_ = self.state
        return self.out_

    def save(self, time):
        self.saved.time.append(time)
        self.saved.rate.append(self.rate)
        self.saved.state.append(self.state)
        self.saved.in_.append(self.in_)
        self.saved.out_.append(self.out_)


class Saved1:
    # For plot 1st order filter savings.
    # A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.state = []
        self.rate = []
        self.in_ = []
        self.out_ = []


class Saved2:
    # For plot 1st order filter savings.
    # A better way is 'Saver' class in pyfilter helpers and requires making a __dict__
    def __init__(self):
        self.time = []
        self.in_ = []
        self.out_ = []
        self.accel = []
        self.AB2_state = []
        self.AB2_rate_state = []
        self.Tustin_state = []
        self.Tustin_rate_state = []


if __name__ == '__main__':
    import sys
    import doctest
    from datetime import datetime
    import numpy as np

    doctest.testmod(sys.modules['__main__'])
    import matplotlib.pyplot as plt


    def overall(filter_1=Saved1(), filter_2=Saved2(), filename='', fig_files=None, plot_title=None, n_fig=None):
        if fig_files is None:
            fig_files = []

        plt.figure()
        n_fig += 1
        plt.subplot(211)
        plt.title(plot_title)
        plt.plot(filter_1.time, filter_1.in_, color='blue', label='in 1')
        plt.plot(filter_1.time, filter_1.out_, color='green', label='out 1')
        plt.legend(loc=3)
        plt.subplot(212)
        plt.plot(filter_2.time, filter_2.in_, color='blue', label='in 2')
        plt.plot(filter_2.time, filter_2.out_, color='green', label='out 2')
        plt.legend(loc=3)
        fig_file_name = filename + "_" + str(n_fig) + ".png"
        fig_files.append(fig_file_name)
        plt.savefig(fig_file_name, format="png")

        return n_fig, fig_files


    class Pulsar:
        def __init__(self):
            self.time_last_hold = 0.
            self.time_last_rest = -100000.
            self.holding = False
            self.resting = True
            self.index = -1
            self.amp = [1.,     0.,     -1.,    1.]
            self.dur = [20.,    0.,     40.,    40.]
            self.rst = [20.,    20.,    40.,    40.]
            self.pulse_value = self.amp[0]
            self.end_time = self.time_end()

        def calculate(self, time):
            if self.resting and time >= self.time_last_rest + self.rst[self.index]:
                if time < self.end_time:
                    self.index += 1
                self.resting = False
                self.holding = True
                self.time_last_hold = time
                self.pulse_value = self.amp[self.index]
            elif self.holding and time >= self.time_last_hold + self.dur[self.index]:
                self.index += 0  # only advance after resting
                self.resting = True
                self.holding = False
                self.time_last_rest = time
                self.pulse_value = 0.
            return self.pulse_value

        def time_end(self):
            time = 0
            for du in self.dur:
                time += du
            for rs in self.rst:
                time += rs
            return time


    def main():
        # Setup to run the transients
        dt = 2
        pull = Pulsar()
        time_end = pull.time_end()
        # time_end = 50.

        filter_1 = LagTustin(dt, 5., -10., 10.)
        # print(filter_1)
        # filter_2 = General2Pole(dt, 1., .707, -10., 10.)
        filter_2 = General2Pole(dt, 0.1, 0.9, -10., 10.)
        # print(filter_2)

        # Executive tasks
        t = np.arange(0, time_end + dt, dt)

        # time loop
        for i in range(len(t)):
            in_ = pull.calculate(t[i])
            reset = (t[i] <= 1)

            # Models
            filter_1.calculate(in_, reset, dt)
            filter_2.calculate(in_, reset, dt)
            # print("t,in,accel,rate_state,state,out=", t[i],filter_2.in_,
            #   filter_2.accel, filter_2.AB2.state, filter_2.Tustin.state, filter_2.out_)

            # Plot stuff
            filter_1.save(t[i])
            filter_2.save(t[i])

        # Data
        # print(filter_1)
        # print(filter_2)

        # Plots
        n_fig = 0
        fig_files = []
        date_time = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
        filename = sys.argv[0].split('/')[-1]
        plot_title = filename + '   ' + date_time

        overall(filter_1.saved, filter_2.saved, filename, fig_files, plot_title=plot_title, n_fig=n_fig)
        plt.show()

    main()
