# Table utility
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
__date__ = '$Date: 2022/01/25 09:42:00 $'

import numpy as np
import Iterate


def binsearch(x_, v_, n):
    # Initialize to high and low
    low = 0
    high = n - 1
    # Check endpoints
    if x_ >= v_[high]:
        low = high
        dx = 0.
    elif x_ <= v_[low]:
        high = low
        dx = 0.
    # else search
    else:
        while high - low > 1:
            mid = int((low + high) / 2)
            if v_[mid] > x_:
                high = mid
            else:
                low = mid
        dx = (x_ - v_[low]) / (v_[high] - v_[low])
    return high, low, dx


# Check if given array is Monotonic
def isMonotonic(A):
    x_, y_ = [], []
    x_.extend(A)
    y_.extend(A)
    x_.sort()
    y_.sort(reverse=True)
    if all(np.array(x_) == np.array(A)) or all(np.array(y_) == np.array(A)):
        return True
    return False


class Error(Exception):
    """Lookup Table Error"""
    pass


class TableInterp1D:
    # Gridded lookup table, natively clipped
    """// 1-D Interpolation Table Lookup
    /* Example usage:
    x = {x1, x2, ...xn}
    v = {v1, v2, ...vn}"""

    def __init__(self, x_, v_):
        self.n = len(x_)
        self.x = x_
        if len(v_) != self.n:
            raise Error("length of array 'v' =", len(v_),
                        " inconsistent with length of 'x' = ", len(x_))
        self.v = v_

    # Interpolation
    def interp(self, x_):
        if self.n < 1:
            return self.v[0]
        high, low, dx = binsearch(x_, self.x, self.n)  # clips
        r_val = self.v[low] + dx * (self.v[high] - self.v[low])
        return float(r_val)

    def __str__(self, prefix='', spacer='  '):
        s = prefix + "(TableInterp1D):\n"
        s += spacer + "x = ["
        count = 1
        N = len(self.x)
        for X in self.x:
            s += " {:7.3f}".format(X)
            if count < N:
                s += ","
                count += 1
        s += "]\n"
        s += spacer + "t = ["
        count = 1
        N = len(self.x)
        for X in self.x:
            s += " {:7.3f}".format(self.interp(X))
            if count < N:
                s += ","
                count += 1
        s += "]\n"
        return s


class TableInterp2D:
    # Gridded lookup table, natively clipped
    """// 2-D Interpolation Table Lookup
    /* Example usage:  see voc_T_ in ../Battery.cpp.
    x = {x1, x2, ...xn}
    y = {y1, y2, ...ym}
    v = {v11, v12, ...v1n, v21, v22, ...v2n, ...............  vm1, vm2, ...vmn}
      = {v11, v12, ...v1n,
         v21, v22, ...v2n,
     ...............
         vm1, vm2, ...vmn}"""

    def __init__(self, x_, y_, v_):
        self.n = len(x_)
        self.m = len(y_)
        self.x = x_
        self.y = y_
        if len(v_) != self.n*self.m:
            raise Error("length of array 'v' =", len(v_),
                        " inconsistent with lengths of 'x' * 'y' = ", len(x_), "*", len(y_))
        self.v = v_
        self.is_monotonic = True
        for j in range(self.m):
            if not isMonotonic(self.v[j*self.n:j*self.n+self.n]):
                self.is_monotonic = False

    def interp(self, x_, y_):
        if self.n < 1 or self.m < 1:
            return self.v[0]
        high1, low1, dx1 = binsearch(x_, self.x, self.n)
        high2, low2, dx2 = binsearch(y_, self.y, self.m)
        temp1 = low2 * self.n + low1
        temp2 = high2 * self.n + low1
        r0 = self.v[temp1] + dx1 * (self.v[low2*self.n + high1] - self.v[temp1])
        r1 = self.v[temp2] + dx1 * (self.v[high2*self.n + high1] - self.v[temp2])
        return float(r0 + dx2 * (r1 - r0))

    # Reverse interpolation
    def r_interp(self, t_, y_):
        ice = Iterate("TableInterp2D.r_interp")
        SOLV_ERR = 1e-12
        SOLV_MAX_COUNTS = 25
        SOLV_SUCC_COUNTS = 4
        if self.is_monotonic:
            xmin = self.x[0]
            xmax = self.x[end]
            ice.init(xmax, xmin, 2*SOLV_ERR)  # init ice.e > SOLV_ERR
            while abs(ice.e) > SOLV_ERR and ice.count < SOLV_MAX_COUNTS and abs(ice.dx) > 0.:
                ice.increment()
                x_ = ice.x
                v_solved = self.interp(x_, y_)
                ice.e(v_solved - t_)
                ice.iterate(verbose=True, success_count=SOLV_SUCC_COUNTS, en_no_soln=True)
            return x_
        else:
            print('r_interp(myTables.py):  table not monotonic in z')
            return np.nan

    def __str__(self, prefix='', spacer='  '):
        s = prefix + "(TableInterp2D):\n"
        s += spacer + "x = ["
        count = 1
        N = len(self.x)
        for X in self.x:
            s += " {:7.3f}".format(X)
            if count < N:
                s += ","
                count += 1
        s += "]\n"
        s += spacer + "y = ["
        count = 1
        N = len(self.y)
        for Y in self.y:
            s += " {:7.3f}".format(Y)
            if count < N:
                s += ","
                count += 1
        s += "]\n"
        s += spacer + "t = ["
        countM = 1
        M = len(self.y)
        for Y in self.y:
            count = 1
            N = len(self.x)
            for X in self.x:
                s += " {:7.3f}".format(self.interp(X, Y))
                if count < N:
                    s += ","
                    count += 1
            if countM < M:
                s += ",\n"
            countM += 1
        s += "]\n"
        return s


if __name__ == '__main__':
    import sys
    import doctest
    import numpy as np
    import timeit
    from scipy.interpolate import RegularGridInterpolator as rg
    doctest.testmod(sys.modules['__main__'])

    t_x_soc = [0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.98, 1.00]
    n = len(t_x_soc)
    t_y_t = [0., 40.]
    m = len(t_y_t)
    t_voc_i = [9.0, 11.8, 12.45, 12.61, 12.8, 12.83, 12.9, 13.00, 13.07, 13.11, 13.23, 13.5,
               9.86, 12.66, 13.31, 13.47, 13.66, 13.69, 13.76, 13.86, 13.93, 13.97, 14.05, 14.4]
    t_voc_l = [[9.0, 11.8, 12.45, 12.61, 12.8, 12.83, 12.9, 13.00, 13.07, 13.11, 13.23, 13.5],
               [9.86, 12.66, 13.31, 13.47, 13.66, 13.69, 13.76, 13.86, 13.93, 13.97, 14.05, 14.4]]
    x = np.array(t_x_soc)
    y = np.array(t_y_t)
    data_interp = t_voc_i
    data_lut = np.array(t_voc_l)

    # local solution
    table2 = TableInterp2D(x, y, data_interp)

    # scipy solution
    lut_voc = rg((x, y), data_lut.T)

    print("Testing local interp")
    for y in t_y_t:
        for x in t_x_soc:
            print('x=', x, 'y=', y, 'table2=', table2.interp(x, y))
    start = timeit.default_timer()
    n = 0
    for y in np.arange(t_y_t[0], t_y_t[-1], .01):
        for x in np.arange(t_x_soc[0], t_x_soc[-1], .01):
            n += 1
            v = table2.interp(x, y)
    end = timeit.default_timer()
    t_in = (end - start)/float(n)
    print('time per call=', t_in)
    print('')

    print("Testing scipy lutt")
    for y in t_y_t:
        for x in t_x_soc:
            print('x=', x, 'y=', y, 'lut_voc=', lut_voc([x, y]))
    start = timeit.default_timer()
    n = 0
    for y in np.arange(t_y_t[0], t_y_t[-1], .01):
        for x in np.arange(t_x_soc[0], t_x_soc[-1], .01):
            n += 1
            v = lut_voc([x, y])
    end = timeit.default_timer()
    t_rg = (end - start)/float(n)
    print('time per call=', t_rg)

    print('ratio=', t_rg/t_in)
    print('')

    t_dv = [-0.9, -0.7, -0.5, -0.3, 0.0, 0.1, 0.3, 0.5, 0.7, 0.9]
    t_soc = [0, .2, .5, 1]
    t_r = [1e-6, 0.021, 0.017, 0.018, 0.006, 0.008, 0.020, 0.026, 0.040, 1e-6,
           1e-6, 1e-6, 0.016, 0.012, 0.002, 0.003, 0.006, 0.006, 1e-6, 1e-6,
           1e-6, 1e-6, 0.016, 0.012, 0.005, 0.006, 0.008, 0.010, 1e-6, 1e-6,
           1e-6, 1e-6, 1e-6, 0.018, 0.005, 0.005, 0.006, 1e-6, 1e-6, 1e-6]
    # x = np.array(t_dv)
    # y = np.array(t_soc)
    # data_interp = t_r

    # local solution
    table2 = TableInterp2D(t_dv, t_soc, t_r)
    print("Testing local interp")
    for y in t_soc:
        for x in t_dv:
            print('x=', x, 'y=', y, 'table2=', table2.interp(x, y))

