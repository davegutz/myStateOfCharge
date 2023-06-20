#!/usr/bin/env python3
#  wcp:  wild copy tool
#  Run in PyCharm
#     or
#  'python3 wcp
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
from configparser import ConfigParser


# Begini - configuration class using .ini files
def parse_tuple(input_):
    return tuple(k.strip() for k in input_[:-1].split(','))


class Begini(ConfigParser):

    def __init__(self, name, def_dict_):
        ConfigParser.__init__(self, converters={'tuple': parse_tuple})

        (config_path, config_basename) = os.path.split(name)
        config_txt = os.path.splitext(config_basename)[0] + '.ini'
        self.config_file_path = os.path.join(config_path, config_txt)
        print('config file', self.config_file_path)
        if os.path.isfile(self.config_file_path):
            self.read(self.config_file_path)
        else:
            cfg_file = open(self.config_file_path, 'w')
            self.read_dict(def_dict_)
            self.write(cfg_file)
            cfg_file.close()
            print('updated', self.config_file_path)

    # Get an item
    def get_item(self, ind, item):
        return self[ind][item]

    # Get a tuple item in list form now
    def get_tuple_item_as_strlist(self, ind, item):
        list_of = self[ind][item]
        list_of_tuple = parse_tuple(list_of)
        list_str = ""
        for str_ in list_of_tuple:
            list_str += '"' + str_ + '"' + ' '
        return list_str

    # Put an item
    def put_item(self, ind, item, value):
        self[ind][item] = value
        self.save_to_file()

    # Put a tuple item
    def put_tuple_item(self, ind, item, tuple_value):
        value_list = ''
        for value in tuple_value:
            value_list += value + ','
        self[ind][item] = value_list
        self.save_to_file()

    # Save again
    def save_to_file(self):
        cfg_file = open(self.config_file_path, 'w')
        self.write(cfg_file)
        cfg_file.close()
        print('updated', self.config_file_path)


def wcp(filepaths=None, silent=False, supported='*'):

    # Configuration for entire folder selection read with filepaths
    def_dict = {'wcp':  {'source': 'source',
                         'target': 'target',
                         'filepaths': ['file1', 'file2', 'file3']},
                }
    cf = Begini(__file__, def_dict)
    # print(list(cf))

    # Make filetypes compatible with askopenfilenames tuple format
    supported_ext = []
    for typ in supported:
        supported_ext.append(('you may enter file type filter press enter', typ))

    # Request list of files in a browse dialog
    if filepaths is None:
        root = tk.Tk()
        root.withdraw()
        filepaths = filedialog.askopenfilenames(title='Please select files', filetypes=supported_ext,
                                                initialfile=cf.get_tuple_item_as_strlist('wcp', 'filepaths'))
        cf.put_tuple_item('wcp', 'filepaths', filepaths)
        if filepaths is None or filepaths == '':
            if silent is False:
                input('\nNo files chosen')
            else:
                messagebox.showinfo(title='Message:', message='No files chosen')
            return None
    source = tk.simpledialog.askstring('wcp source target', 'source string', initialvalue=cf.get_item('wcp', 'source'))
    cf.put_item('wcp', 'source', source)
    target = tk.simpledialog.askstring('wcp source target', 'target string', initialvalue=cf.get_item('wcp', 'target'))
    cf.put_item('wcp', 'target', target)
    for filepath in filepaths:
        destpath = filepath.replace(source, target)
        if destpath != filepath:
            if os.path.exists(filepath):
                shutil.copyfile(filepath, destpath)
                shutil.copystat(filepath, destpath)
                print('copied', filepath, 'to', destpath)
            else:
                print(filepath, 'not found')
        else:
            print("did not find source= '", source, "' in", filepath)


if __name__ == '__main__':
    def main():
        wcp()

    # Main call
    main()
