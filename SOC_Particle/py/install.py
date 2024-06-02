#  install.py
#  2024-Apr-13  Dave Gutz   Create
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
from test_soc_util import run_shell_cmd
from Colors import Colors
import shutil
import sys
import os

test_cmd_create = None
test_cmd_copy = None
result = None
GUI_TestSOC_dest_path = None

# Create executable
GUI_TestSOC_path = os.path.join(os.getcwd(), 'GUI_TestSOC.png')
GUI_TestSOC_Icon_path = os.path.join(os.getcwd(), 'GUI_TestSOC_Icon.png')
if sys.platform == 'win32':
    GUI_TestSOC_dest_path = os.path.join(os.getcwd(), 'dist', 'GUI_TestSOC', '_internal', 'GUI_TestSOC.png')
    GUI_TestSOC_Icon_dest_path = os.path.join(os.getcwd(), 'dist', 'GUI_TestSOC', '_internal', 'GUI_TestSOC_Icon.png')
    test_cmd_create = 'pyinstaller .\\GUI_TestSOC.py --i GUI_TestSOC.ico -y'
    result = run_shell_cmd(test_cmd_create, silent=False)
    if result == -1:
        print(Colors.fg.red, 'failed', Colors.reset)
        exit(1)
    else:
        print(Colors.fg.green, 'success', Colors.reset)

    # Provide dependencies
    shutil.copyfile(GUI_TestSOC_path, GUI_TestSOC_dest_path)
    shutil.copystat(GUI_TestSOC_path, GUI_TestSOC_dest_path)
    print(Colors.fg.green, "copied files", Colors.reset)

# Install as deeply as possible
test_cmd_install = None
if sys.platform == 'linux':
    login = os.getlogin()
    desktop_entry = f"""[Desktop Entry]
Name=GUI_TestSOC
Exec=/home/{login}/Documents/GitHub/myStateOfCharge/SOC_Particle/py/venv/bin/python3 /home/{login}/Documents/GitHub/myStateOfCharge/SOC_Particle/py/GUI_TestSOC.py
Path=/home/{login}/Documents/GitHub/myStateOfCharge/SOC_Particle/py
Icon=/home/{login}/Documents/GitHub/myStateOfCharge/SOC_Particle/py/GUI_TestSOC.ico
comment=app
Type=Application
Terminal=true
Encoding=UTF-8
Categories=Utility
"""
    with open(f"/home/{login}/Desktop/GUI_TestSOC.desktop", "w") as text_file:
        result = text_file.write("%s" % desktop_entry)
    if result == -1:
        print(Colors.fg.red, 'failed', Colors.reset)
    else:
        print(Colors.fg.green, 'success', Colors.reset)

    #  Launch permission
    test_cmd_launch = f'gio set /home/{login}/Desktop/GUI_TestSOC.desktop metadata::trusted true'
    result = run_shell_cmd(test_cmd_launch, silent=False)
    if result == -1:
        print(Colors.fg.red, 'gio set failed', Colors.reset)
    else:
        print(Colors.fg.green, 'gio set success', Colors.reset)
    test_cmd_perm = 'chmod a+x ~/Desktop/GUI_TestSOC.desktop'
    result = run_shell_cmd(test_cmd_perm, silent=False)
    if result == -1:
        print(Colors.fg.red, 'failed', Colors.reset)
    else:
        print(Colors.fg.green, 'success', Colors.reset)

    # Execute permission
    test_cmd_perm = 'chmod a+x ~/Desktop/GUI_TestSOC.desktop'
    result = run_shell_cmd(test_cmd_perm, silent=False)
    if result == -1:
        print(Colors.fg.red, f"'chmod ...' failed code {result}", Colors.reset)
    else:
        print(Colors.fg.green, 'chmod success', Colors.reset)

    # Move file
    try:
        # pass
        result = shutil.move(f'/home/{login}/Desktop/GUI_TestSOC.desktop',
                             '/usr/share/applications/GUI_TestSOC.desktop')
    except PermissionError:
        print(Colors.fg.red, "Stop and establish sudo permissions", Colors.reset)
        print(Colors.fg.red, "  or", Colors.reset)
        print(Colors.fg.red, f"sudo mv /home/{login}//Desktop/GUI_TestSOC.desktop /usr/share/applications/.", Colors.reset)
        exit(1)
    if result != '/usr/share/applications/GUI_TestSOC.desktop':
        print(Colors.fg.red, f"'mv ...' failed code {result}", Colors.reset)
    else:
        print(Colors.fg.green, 'mv success.  Browse apps :: and make it favorites.  Open and set path to dataReduction', Colors.reset)
        print(Colors.fg.green, "you shouldn't have to remake shortcuts", Colors.reset)
elif sys.platform == 'darwin':
    print(Colors.fg.green,
          f"Make sure 'Python Launcher' (Python Script Preferences) option for 'Allow override with #! in script' is checked.\n"
          f"in Finder ctrl-click on 'GUI_TestSOC.py'\n"
          f"   - Open and copy icon into paste buffer.\n"
          f"   - Then 'Get Info' on the duplicate, click on 2nd icon, paste.   Drag item to taskbar.",
          Colors.reset)
elif sys.platform == 'win32':
    print(Colors.fg.green, f"browse to executable in 'dist/GUI_TestSOC' and double-click.  Create shortcut first time", Colors.reset)
    print(Colors.fg.green, "you shouldn't have to remake shortcuts", Colors.reset)
