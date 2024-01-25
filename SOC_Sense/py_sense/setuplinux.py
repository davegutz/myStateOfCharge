#  setuplinux:  script to make desktop shortcut
#  2023-Jun-02  Dave Gutz   Create
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

# usage:
# pip install pyshortcuts
# run 'python setuplinux.py'
#  or
# run this file in pyCharm as is
from pyshortcuts import make_shortcut

make_shortcut('GUI_TestSOC.py', name='GUI_TestSOC')
