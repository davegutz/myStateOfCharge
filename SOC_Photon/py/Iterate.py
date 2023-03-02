# Iterate class to provide a general purpose solver.   Begins with successive approximation and finishes with Newton-Rapheson
# Copyright (C) 2023 Dave Gutz
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
__date__ = '$Date: 2023/03/02 13:15:02 $'

import numpy as np


class Iterator:
    def __init__(self, desc=''):
        self.count = 0
        self.desc = desc
        self.de = 0.
        self.des = 0.
        self.dx = 0.
        self.e = 0.
        self.ep = 0.
        self.limited = False
        self.x = 0.
        self.xmax = 0.
        self.xmin = 0.
        self.xp = 0.

    # Initialize
    def init(self, xmax=0., xmin=0., eInit=0.):
        self.xmax = xmax
        self.xmin = xmin
        self.e = eInit
        self.ep = self.e
        self.xp = self.xmax
        self.x = self.xmin  # Do min and max first
        self.dx = self.x - self.xp
        self.de = self.e - self.ep
        self.count = 0

    # Generic iteration calculation, method of successive approximations for
    # success_count then Newton-Rapheson as needed - works with iterateInit.
    # Inputs:  self.e
    # Outputs:  self.x
    def iterate(self, verbose=False, success_count=0, en_no_soln=False):
        self.de = self.e - self.ep
        self.des = np.sign(self.de)*max(abs(self.de), 1e-16)
        self.dx = self.x - self.xp
        if verbose:
            s = self.desc + '(' + "{:2d}".format(self.count) + ':'
            s += " xmin{:12.8f}".format(self.xmin)
            s += " xmax{:12.8f}".format(self.xmax)
            s += " e{:12.8f}".format(self.e)
            s += " des{:12.8f}".format(self.des)
            s += " dx{:12.8f}".format(self.dx)
            s += " de{:12.8f}".format(self.de)
            s += " e{:12.8f}".format(self.e)
            # print(s)

        # Check min max sign change
        no_soln = False
        if self.count == 2:
            if self.e*self.ep >= 0 and en_no_soln:  # No solution possible
                no_soln = True
            if abs(self.ep) < abs(self.e):
                self.x = self.xp
            self.ep = self.e
            self.limited = False
            if verbose:
                print(self.desc, ':No soln')  # Leaving x at most likely limit value and recalculating...
            return self.e
        else:
            no_soln = False

        # Stop after recalc and no_soln
        if self.count == 3 and no_soln is True:
            self.e = 0.
            return self.e
        self.xp = self.x
        self.ep = self.e
        if self.count == 1:
            self.x = self.xmax  # Do min and max first
        else:
            if self.count > success_count:
                self.x = max(min(self.x - self.e/self.des*self.dx, self.xmax), self.xmin)
                if self.e > 0:
                    self.xmax = self.xp
                else:
                    self.xmin = self.xp
            else:
                if self.e > 0:
                    self.xmax = self.xp
                    self.x = (self.xmin + self.x) / 2.
                else:
                    self.xmin = self.xp
                    self.x = (self.xmax + self.x) / 2.
            if self.x == self.xmax or self.x == self.xmin:
                self.limited = False
            else:
                self.limited = True
        return self.e
