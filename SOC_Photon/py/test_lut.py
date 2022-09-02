# lookup_table - multidimensional lookup table class
# Copyright (C) 2007 RADLogic
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
"""Define a multidimensional lookup table class.

This uses (piecewise) linear interpolation for table lookups.

- axes are defined in order, and can be added in stages
- axes are either purely increasing or purely decreasing
- this module is units-agnostic

This is intended for use with lookup tables compiled from Liberty files,
so must meet the needs for that application.

It might be useful for other applications as well.

This is designed to be compatible with Python 2.2.

The unittests (doctest) can be run by running this script directly with Python:
python lookup_table.py

Make sure the tests pass before checking in changes to this module.

This module is probably a good candidate for releasing as open source.
(concise, well-defined scope, doctests, finished, would benefit from many eyes)

"""

__author__ = 'Tim Wegener <twegener@radlogic.com.au>'
__version__ = '$Revision: 1.1 $'
__date__ = '$Date: 2010/12/17 13:15:02 $'

from pyDAGx.lookup_table import LookupTable

def crosscheck3d():
    """This is just here for testing/debug purposes."""
    t_dv = [-0.9, -0.7,     -0.5,   -0.3,   0.0,    0.3,    0.5,    0.7,    0.9]
    t_soc = [0, .5, 1]
    t_r = [1e-6, 0.064,    0.050,  0.036,  0.015,  0.024,  0.030,  0.046,  1e-6,
           1e-6, 1e-6,     0.050,  0.036,  0.015,  0.024,  0.030,  1e-6,   1e-6,
           1e-6, 1e-6,     1e-6,   0.036,  0.015,  0.024,  1e-6,   1e-6,   1e-6]
    lut = LookupTable(clip_x=True)
    lut.addAxis('x', t_dv)
    lut.addAxis('y', t_soc)
    lut.setValueTable(t_r)
    print("expected 1e-6 got", lut.lookup(x=-.902, y=0.0957))
    print("expected 1e-6 got", lut.lookup(x=-.9, y=0))
    print("expected 1e-6 got", lut.lookup(x=.902, y=0.0957))
    print("expected 1e-6 got", lut.lookup(x=.9, y=0))


def main():
    crosscheck3d()


if __name__ == '__main__':
    import sys
    main()
