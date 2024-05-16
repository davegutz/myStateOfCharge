# Battery - general purpose battery class for modeling
# Copyright (C) 2024 Dave Gutz
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

"""Depending on platform locate the temp folders."""

import os
import platform


def local_paths(version_folder='no_name'):
    path_to_local = None
    if platform.system() == 'Linux':
        path_to_local = '/home/daveg/.local/SOC_Particle'
    elif platform.system() == 'Darwin':
        path_to_local = '/Users/daveg/Library/SOC_Particle'
    else:
        path_to_local = os.path.join(os.getenv('LOCALAPPDATA'), 'SOC_Particle')

    if not os.path.isdir(path_to_local):
        os.mkdir(path_to_local)
    path_to_version_folder = os.path.join(path_to_local, version_folder)
    if not os.path.isdir(path_to_version_folder):
        os.mkdir(path_to_version_folder)
    path_to_temp = os.path.join(path_to_version_folder, 'temp')
    if not os.path.isdir(path_to_temp):
        os.mkdir(path_to_temp)
    save_pdf_path = os.path.join(path_to_version_folder, 'figures')
    if not os.path.isdir(save_pdf_path):
        os.mkdir(save_pdf_path)
    putty_test_csv_path = os.path.join(path_to_local, 'putty_test.csv')

    return path_to_temp, save_pdf_path, putty_test_csv_path


def version_from_data_file(path_to_data):
    data_file_folder = os.path.split(path_to_data)[0]
    version = os.path.split(data_file_folder)[-1]
    return version


def version_from_data_path(path):
    version = os.path.split(path)[-1]
    return version
