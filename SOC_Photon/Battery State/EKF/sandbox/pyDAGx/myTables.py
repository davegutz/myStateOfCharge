# Hysteresis class to model battery charging / discharge hysteresis
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
__date__ = '$Date: 2022/01/25 09:42:00 $'

def binsearch(x, v, n):
    # Initialize to high and low
    low = 0
    high = n - 1
    # Check endpoints
    if x >= v[high]:
        low = high
        dx = 0.
    elif x <= v[low]:
        high = low
        dx = 0.
    # else search
    else:
        while high - low > 1:
            mid = int((low + high) / 2)
            if v[mid] > x:
                high = mid
            else:
                low = mid
        dx = (x - v[low]) / (v[high] - v[low])
    return high, low, dx


class Error(Exception):
    """Lookup Table Error"""
    pass


class TableInterp1D():
    # Gridded lookup table, natively clipped
    """// 1-D Interpolation Table Lookup
    /* Example usage:
    x = {x1, x2, ...xn}
    v = {v1, v2, ...vn}"""

    def __init__(self, x, v):
        self.n = len(x)
        self.x = x
        if len(v) != self.n:
            raise Error("length of array 'v' =", len(v),
                        " inconsistent with length of 'x' = ", len(x))
        self.v = v

    def interp(self, x):
        if self.n < 1:
            return self.v[0]
        high, low, dx = binsearch(x, self.x, self.n)
        r_val = self.v[low] + dx * (self.v[high] - self.v[low])
        return float(r_val)


class TableInterp2D():
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

    def __init__(self, x, y, v):
        self.n = len(x)
        self.m = len(y)
        self.x = x
        self.y = y
        if len(v) != self.n*self.m:
            raise Error("length of array 'v' =", len(v),
                        " inconsistent with lengths of 'x' * 'y' = ", len(x), "*", len(y))
        self.v = v

    def interp(self, x, y):
        if self.n < 1 or self.m < 1:
            return self.v[0]
        high1, low1, dx1 = binsearch(x, self.x, self.n)
        high2, low2, dx2 = binsearch(y, self.y, self.m)
        temp1 = low2 * self.n + low1
        temp2 = high2 * self.n + low1
        r0 = self.v[temp1] + dx1 * ( self.v[low2*self.n + high1] - self.v[temp1])
        r1 = self.v[temp2] + dx1 * ( self.v[high2*self.n + high1] - self.v[temp2])
        return float(r0 + dx2 * (r1 - r0))


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

    print("Testing scipy interp")
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
