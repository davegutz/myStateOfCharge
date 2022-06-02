# Delay class debounce logic
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
__date__ = '$Date: 2022/06/02 13:15:02 $'


class TFDelay:
    # Use variable resistor to create hysteresis from an RC circuit

    def __init__(self, in_=False, t_true=0., t_false=0., dt=0.1):
        # Defaults
        self.timer = 0.
        self.nt = int(max(round(t_true/dt)+1, 0))
        self.nf = int(max(round(t_false/dt)+1, 0))
        self.dt = dt
        if t_true == 0.0:
            self.nt = 0
        if t_false == 0.0:
            self.nf = 0
        if in_:
            self.timer = self.nf
        else:
            self.time = -self.nt

    def calculate1(self, in_):
        if self.timer >= 0.:
            if in_:
                self.timer = self.nf
            else:
                self.timer -= 1
                if self.timer < 0:
                    self.timer = -self.nt
        else:
            if not in_:
                self.timer = -self.nt
            else:
                self.timer += 1
                if self.timer >= 0.:
                    self.timer = self.nf
        return self.timer > 0.

    def calculate2(self, in_, reset):
        if reset:
            if in_:
                self.timer = self.nf
            else:
                self.timer = -self.nt
            out = in_
        else:
            out = self.calculate1(in_)
        return out

    def calculate3(self, in_, t_true, t_false):
        self.nt = int(max(round(t_true / self.dt), 0))
        self.nf = int(max(round(t_false / self.dt), 0))
        return self.calculate1(in_)

    def calculate4t(self, in_, t_true, t_false, dt):
        self.dt = dt
        self.nt = int(max(round(t_true / self.dt), 0))
        self.nf = int(max(round(t_false / self.dt), 0))
        return self.calculate1(in_)

    def calculate4r(self, in_, t_true, t_false, reset):
        if reset > 0:
            if in_:
                self.timer = self.nf
            else:
                self.timer = -self.nt
        return self.calculate3(in_, t_true, t_false)

    def calculate(self, in_, t_true, t_false, dt, reset):
        if reset:
            if in_:
                self.timer = self.nf
            else:
                self.timer = -self.nt
        return self.calculate4t(in_, t_true, t_false, dt)
