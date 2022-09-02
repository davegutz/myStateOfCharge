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

from bisect import bisect_right


class Error(Exception):
    """Lookup Table Error"""
    pass


class LookupTable:
    """Multidimensional lookup table

    1-axis tests:

    >>> lut = LookupTable()
    >>> x_axis_values = [1., 2., 3., 4., 5.]
    >>> lut.addAxis('x', x_axis_values)

    >>> def example_1var_func(x):
    ...     return (3 * x) + 1
    >>> lut.setValueTable([example_1var_func(x) for x in x_axis_values])

    Normal situation:
    
    >>> print(example_1var_func(2.5))
    8.5

    >>> print(lut.lookup(x=2.5))
    8.5

    Lookup a value on the axis value:
    >>> print(lut.lookup(x=3.))
    10.0

    Lookup a value on the bottom axis value:
    >>> print(lut.lookup(x=1.))
    4.0

    Lookup a value on the top axis value:
    >>> print(lut.lookup(x=5.))
    16.0

    Lookup a value below the bottom axis value:
    >>> print(lut.lookup(x=0.5))
    2.5

    Lookup a value above the top axis value:
    >>> print(lut.lookup(x=6.))
    19.0

    Lookup a value without specifying an axis value:
    >>> print(lut.lookup())
    Traceback (most recent call last):
    Error: No axis value for 'x'

    >>> print(lut.lookup(x=None))
    Traceback (most recent call last):
    Error: No axis value for 'x'

    Try to include a non-existent axis value as well:
    >>> print(lut.lookup(x=2.5, y=10))
    8.5

    2-axis tests:

    >>> lut = LookupTable()
    >>> x_axis_values = [1., 2., 3., 4., 5.]
    >>> y_axis_values = [1., 2., 3., 4., 5.]
    >>> lut.addAxis('x', x_axis_values)
    >>> lut.addAxis('y', y_axis_values)

    Use a non-linear function!
    >>> def example_2var_func(x, y):
    ...     return (2 * x * x) + (3 * y) + 1
    >>> lut.setValueTable([[example_2var_func(x, y) for y in y_axis_values]
    ...                    for x in x_axis_values])

    Normal situation:
    
    >>> print(example_2var_func(x=2.625, y=1.1))
    18.08125

    Nearest neighbours:

    >>> x1 = 2.
    >>> x2 = 3.
    >>> y1 = 1.
    >>> y2 = 2.

    >>> x1y1 = example_2var_func(x=x1, y=y1)
    >>> x2y1 = example_2var_func(x=x2, y=y1)
    >>> x1y2 = example_2var_func(x=x1, y=y2)
    >>> x2y2 = example_2var_func(x=x2, y=y2)

    >>> print(x1y1)
    12.0

    >>> print(lut.lookup(x=x1, y=y1))
    12.0

    >>> print(x2y1)
    22.0

    >>> print(lut.lookup(x=x2, y=y1) )
    22.0

    >>> print(x1y2)
    15.0

    >>> print(lut.lookup(x=x1, y=y2))
    15.0

    >>> print(x2y2)
    25.0

    >>> print(lut.lookup(x=x2, y=y2))
    25.0

    Now find the intermediate value points for each x value.

    >>> x = 2.625
    >>> y = 1.1
    >>> val_x1 = (x1y2-x1y1) * ((y-y1)/(y2-y1)) + x1y1
    >>> print(val_x1)
    12.3
    
    >>> print(lut.lookup(x=x1, y=y))
    12.3

    >>> val_x2 = (x2y2-x2y1) * ((y-y1)/(y2-y1)) + x2y1
    >>> print(val_x2)
    22.3
    
    >>> print(lut.lookup(x=x2, y=y))
    22.3

    Now interpolate the intermediate values to find the final value.

    >>> print(((x-x1) * ((val_x2 - val_x1)/(x2-x1))) + val_x1)
    18.55

    >>> print(lut.lookup(x=x, y=y))
    18.55

    Lookup a value on the bottom axis value:
    >>> print(lut.lookup(x=1., y=1.))
    6.0

    Lookup a value on the top axis value:
    >>> print(lut.lookup(x=5., y=5.))
    66.0

    Lookup a value below the bottom axis value:
    >>> print(lut.lookup(x=0.2, y=0.1))
    -1.5

    Lookup a value above the top axis value:
    >>> print(lut.lookup(x=6., y=5.5))
    85.5

    Different sized axes:

    Use a linear function this time.
    
    >>> lut = LookupTable()
    >>> x_axis_values = [2., 4., 8., 16., 32.]
    >>> y_axis_values = [1., 1.9, 6., 7.]
    >>> lut.addAxis('x', x_axis_values)
    >>> lut.addAxis('y', y_axis_values)
    >>> def example_2var_func(x, y):
    ...     return (2 * x) + (3 * y) + 1
    >>> value_table = []
    >>> for x in x_axis_values:
    ...     row = []
    ...     for y in y_axis_values:
    ...         row.append(example_2var_func(x, y))
    ...     value_table.append(row)
    >>> lut.setValueTable(value_table)

    ## >>> lut.setValueTable([[example_2var_func(x, y) for y in y_axis_values]
    ## ...                    for x in x_axis_values])
    >>> ## print('lutv', lut.value_table
    
    Normal situation:
    
    >>> print(example_2var_func(x=2.4, y=1.1))
    9.1

    >>> print(lut.lookup(x=2.4, y=1.1))
    9.1

    Lookup a value on the axis value:
    >>> print(lut.lookup(x=8., y=1.9))
    22.7

    Lookup a value on the bottom axis value:
    >>> print(lut.lookup(x=2., y=1.))
    8.0

    Lookup a value on the top axis value:
    >>> print(lut.lookup(x=32., y=7.))
    86.0
    
    Lookup a value below the bottom axis value:
    >>> print(lut.lookup(x=0.2, y=0.1))
    1.7

    Lookup a value above the top axis value:
    >>> print(lut.lookup(x=40., y=10.))
    111.0

    3-axis tests:

    >>> lut = LookupTable()
    >>> x_axis_values = [1., 2., 3., 4., 5.]
    >>> y_axis_values = [1., 2., 3., 4., 5.]
    >>> z_axis_values = [1., 2., 3., 4., 5.]
    >>> lut.addAxis('x', x_axis_values)
    >>> lut.addAxis('y', y_axis_values)
    >>> lut.addAxis('z', z_axis_values)

    >>> def example_3var_func(x, y, z):
    ...     return (2 * x) + (3 * y) + (4 * z) + 1
    >>> lut.setValueTable([[[example_3var_func(x, y, z) for z in z_axis_values]
    ...                     for y in y_axis_values]
    ...                    for x in x_axis_values])

    Normal situation:
    
    >>> print(example_3var_func(x=2.4, y=1.1, z=3.3))
    22.3

    >>> print(lut.lookup(x=2.4, y=1.1, z=3.3))
    22.3

    """

    def __init__(self, clip_x=None):

        # axis_names - map name->index
        self.axis_names = {}
        self.clip_x = clip_x

        # axes - define independent variable values known along each axis
        #    [[x0, x1, ..., xn], [y0, y1, ..., yn], ...]
        self.axes = []

        # value_table - store dependent variable values for each point in the
        #               table
        #    [[[val_x0_y0_...z0, val_x0_y0..._z1, ...], [], ...], ...]
        self.value_table = [] 

    def addAxis(self, name, axis_values=None):
        """Add an axis definition."""

        if name in self.axis_names:
            raise Error("Axis already exists with name: '%s'" % name)
        axis_i = len(self.axes)
        self.axis_names[name] = axis_i
        self.axes.append(axis_values)

    def setAxisValues(self, axis_name, axis_values):
        """Set the axis values for the specified axis.

        Axis values define points along the axis at which measurements
        were taken.

        This will raise an error if the value table already exists.

        # todo: Add doctests.

        """
        # todo: Is raising an error here necessary?
        if self.value_table is not None:
            raise Error("Cannot define axis once value table has been set.")
        axis_i = self.axis_names[axis_name]
        # if len(axis_values) != len(self.axes[axis_i]):
        # print('warning: number of axis values changed'
        self.axes[axis_i] = axis_values

    def setValueTable(self, value_table):
        """Set the value table to the specified sequence of sequences.

        Nesting should correspond to value_table[axis0_i][axis1_i]...[axisn_i]

        """
        self.value_table = value_table

    def getAxisName(self, axis_i):
        """Return the name of the specified axis. (Index starts at 0)"""

        result = None
        for name, i in self.axis_names.items():
            if i == axis_i:
                result = name
                break
        return result

    def validate(self):
        """Check whether value table matches axes and do other sanity checks.

        Return True if valid, False if not.

        """
        valid = True

        # Check that axis values are purely increasing or purely decreasing.
        for axis in self.axes:
            adjacent_axis_values = zip(axis[:-1], axis[1:])
            delta_signs = [cmp(x, x_next)
                            for x, x_next in adjacent_axis_values]
            # todo: Generate error messages? Perhaps to supplied file object.
            if 0 in delta_signs:
                valid = False
            elif -1 in delta_signs and 1 in delta_signs:
                valid = False

        # Check that value_table size matches the axis definitions.
        axis_size = tuple([len(axis) for axis in self.axes])

        # todo: Handle possible exception.
        table_size = nestedSequenceSize(self.value_table)
        
        if table_size != axis_size:
            # todo: Error message.
            valid = False

        return valid

    def lookup(self, **kwargs):
        """Lookup the interpolated value for given axis values.

        Arguments:
        Specify the axis values for the lookup, using the axis names as
        keyword arguments.
        
        """
        # Check that a value table exists.
        if not self.value_table:
            raise Error("No values set for lookup table")

        # Check that axis values have been specified.
        for axis_name in self.axis_names.keys():
            if kwargs.get(axis_name) is None:
                raise Error("No axis value for '%s'" % axis_name)
        
        # axis_values -- [x, y, ...] for which to find value
        axis_values = [None] * len(self.axes)

        # nearset_indexes -- [x1_i, x2_i, ...]
        #                     table indexes for nearest (on the left if
        #                     possible) table value
        #                    (add 1 for the index of the right side)
        nearest_indexes = [None] * len(self.axes)
        
        for axis_name, axis_value in kwargs.items():
            try:
                axis_i = self.axis_names[axis_name]
            except KeyError:
                # Ignore axis name/values for axes that do not exist.
                # todo: Is this the right way to go?
                continue
            axis = self.axes[axis_i]

            axis_values[axis_i] = axis_value

            interval_start_i = bisect_right(axis, axis_value) - 1
            # Ensure there is always one point after the interval start point.
            if interval_start_i > (len(axis) - 2):
                interval_start_i = len(axis) - 2
            elif interval_start_i < 0:
                interval_start_i = 0

            nearest_indexes[axis_i] = interval_start_i

        #         print('axis_values', axis_values
        #         print('nearest_indexes', nearest_indexes
        #         print('value_table', self.value_table
        
        # Need to interpolate on this data.
        return self.interp_n(axis_values, nearest_indexes, self.value_table)
            
    def interp_n(self, axis_values, nearest_indexes, value_table):
        """Linearly interpolate across multiple dimensions.

        axis_values -- tuple of axis coords for which to find the value
                       (x, y, ...)
        nearest_indexes -- [x1_i, x2_i, ...]
                           table indexes for nearest (on the left if
                           possible) table value
        value_table -- list of lists containing a table of values
                       first (left-most) index corresponds to the x-axis

        """
        # Do the parts required for recursion first.
        x1_i = nearest_indexes[0]
        x2_i = x1_i + 1

        # Represent the other axis variables as y.
        y1 = value_table[x1_i]
        y2 = value_table[x2_i]

        if hasattr(y1, '__len__'):
            # The value still depends on other axes.
            # Need to recurse, to interpolate on the subspace
            y1 = self.interp_n(axis_values[1:], nearest_indexes[1:], y1)
            y2 = self.interp_n(axis_values[1:], nearest_indexes[1:], y2)
            
        x = axis_values[0]
        axis = self.axes[len(self.axes) - len(axis_values)]
        x1 = axis[x1_i]
        x2 = axis[x2_i]

        # Linearly interpolate x on the line from (x1, y1) to (x2, y2).
        # interp(x, (x1, y1), (x2, y2))
        import numpy as np
        slope = (y2 - y1) / (x2 - x1)
        if self.clip_x:
            x = min(max(x, x1), x2)
        # val = slope * (x - x1) + y1
        val = ( y2*(x-x1) + y1*(x2-x) ) / (x2 - x1)

        return val


def nestedSequenceSize(nested_sequence):
    """Return tuple of the size of each level of nested sequence.

    Strings and not treated as sequences.

    Raise an error if the sizes at one level are not equal.

    """
    level_len = len(nested_sequence)
    sub_sequence_sizes = []
    for sub_sequence in nested_sequence:
        if (hasattr(sub_sequence, '__len__') and not isinstance(sub_sequence, str, unicode)):
            sub_sequence_size = nestedSequenceSize(sub_sequence)
        else:
            sub_sequence_size = None
        sub_sequence_sizes.append(sub_sequence_size)

    # Check that sub_sequence sizes are equal.
    first_sub_sequence_size = sub_sequence_sizes[0]
    for sub_sequence_size in sub_sequence_sizes[1:]:
        if sub_sequence_size != first_sub_sequence_size:
            raise Error("Different sub-sequence sizes found at same level")

    if first_sub_sequence_size is None:
        return level_len,
    else:
        return first_sub_sequence_size + level_len,

def crosscheck3d():
    """This is just here for testing/debug purposes."""
    t_dv = [-0.9, -0.7,     -0.5,   -0.3,   0.0,    0.3,    0.5,    0.7,    0.9]
    t_soc = [0, .5, 1]
    t_r = [1e-6, 0.064,    0.050,  0.036,  0.015,  0.024,  0.030,  0.046,  1e-6,
           1e-6, 1e-6,     0.050,  0.036,  0.015,  0.024,  0.030,  1e-6,   1e-6,
           1e-6, 1e-6,     1e-6,   0.036,  0.015,  0.024,  1e-6,   1e-6,   1e-6]
    lut = LookupTable()
    lut.addAxis('x', t_dv)
    lut.addAxis('y', t_soc)
    lut.setValueTable(t_r)
    res = lut.lookup(x=-.9, y=0)
    ev = 1e-6
    print("ev=", ev, "got", res)
    res = lut.lookup(x=-.902, y=0.0957)
    ev = 1e-6
    print("ev=", ev, "got", res)

def crosscheck2d():
    """This is just here for testing/debug purposes.

    # todo: Integrate this into the unittests somehow?

    """
    lut = LookupTable()
    x_axis_values = [1., 2., 3., 4., 5.]
    y_axis_values = [1., 2., 3., 4., 5.]
    lut.addAxis('x', x_axis_values)
    lut.addAxis('y', y_axis_values)

    def func(x, y):
        return (2 * x * x) + (3 * y) + 1

    lut.setValueTable([[func(x, y) for y in y_axis_values]
                       for x in x_axis_values])

    x1 = 4.
    x2 = 5.
    y1 = 4.
    y2 = 5.

    x = 6.
    y = 5.5

    # Nearest neighbours
    x1y1 = func(x=x1, y=y1)
    x2y1 = func(x=x2, y=y1)
    x1y2 = func(x=x1, y=y2)
    x2y2 = func(x=x2, y=y2)

    # Now find the intermediate value points for each x value.

    val_x1 = (x1y2-x1y1) * ((y-y1)/(y2-y1)) + x1y1
    print('val_x1', val_x1)

    print(lut.lookup(x=x1, y=y))

    val_x2 = (x2y2-x2y1) * ((y-y1)/(y2-y1)) + x2y1
    print('val_x2', val_x2
          )
    print(lut.lookup(x=x2, y=y))

    # Now interpolate the intermediate values to find the final value.

    print('expected', ((x-x1) * ((val_x2 - val_x1)/(x2-x1))) + val_x1)

    print(lut.lookup(x=x, y=y))


def main():
    crosscheck2d()


if __name__ == '__main__':
    import sys
    import doctest
    doctest.testmod(sys.modules['__main__'])
    main()
