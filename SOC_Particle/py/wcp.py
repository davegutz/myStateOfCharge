#  wcp:  wild copy tool
#  Run in PyCharm
#     or
#  'python wcp.py
#
#  2023-Jun-15  Dave Gutz   Create
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

"""Use wildcards to copy new files"""

import os
import shutil
from tkinter import filedialog, messagebox, simpledialog
import tkinter as tk


def wcp(filepaths=None, silent=False, supported='*'):

    # Make filetypes compatible with askopenfilenames tuple format
    supported_ext = []
    for typ in supported:
        supported_ext.append(('you may enter file type filter press enter', typ))

    # Request list of files in a browse dialog
    if filepaths is None:
        root = tk.Tk()
        root.withdraw()
        filepaths = filedialog.askopenfilenames(title='Please select files', filetypes=supported_ext)
        if filepaths is None or filepaths == '':
            if silent is False:
                input('\nNo files chosen')
            else:
                messagebox.showinfo(title='Message:', message='No files chosen')
            return None
    source = tk.simpledialog.askstring('wcp source target', 'source string')
    target = tk.simpledialog.askstring('wcp source target', 'target string')
    for filepath in filepaths:
        destpath = filepath.replace(source, target)
        shutil.copyfile(filepath, destpath)
        shutil.copystat(filepath, destpath)
        print('copied', filepath, 'to', destpath)


if __name__ == '__main__':
    def main():
        wcp()

    # Main call
    main()
