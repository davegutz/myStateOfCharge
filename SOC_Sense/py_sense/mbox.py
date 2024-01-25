#  Message boxes using tkinter
#
#  2023-Apr-29  Dave Gutz   Create from Honest Abe at Stackoverflow
#  https://stackoverflow.com/questions/10057672/correct-way-to-implement-a-custom-popup-tkinter-dialog-box
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

import tkinter


# Dialog for application with a main GUI
class Mbox(object):

    root = None

    def __init__(self, msg, dict_key=None):
        """
        msg = <str> the message to be displayed
        dict_key = <sequence> (dictionary, key) to associate with user input
        (providing a sequence for dict_key creates an entry for user input)
        """
        tki = tkinter
        self.top = tki.Toplevel(Mbox.root)

        frm = tki.Frame(self.top, borderwidth=4, relief='ridge')
        frm.pack(fill='both', expand=True)

        label = tki.Label(frm, text=msg)
        label.pack(padx=4, pady=4)

        caller_wants_an_entry = dict_key is not None

        if caller_wants_an_entry:
            self.entry = tki.Entry(frm)
            self.entry.pack(pady=4)

            b_submit = tki.Button(frm, text='Submit')
            b_submit['command'] = lambda: self.entry_to_dict(dict_key)
            b_submit.pack()

        b_cancel = tki.Button(frm, text='Cancel')
        b_cancel['command'] = self.top.destroy
        b_cancel.pack(padx=4, pady=4)

    def entry_to_dict(self, dict_key):
        data = self.entry.get()
        if data:
            d, key = dict_key
            d[key] = data
            self.top.destroy()


# Dialog for application without a main GUI
class MessageBox(object):

    def __init__(self, msg, b1, b2, b3, b4, b5, frame, t, entry):
        root = self.root = tkinter.Tk()
        root.title('Message')
        self.msg = str(msg)
        self.returning = None

        # ctrl+c to copy self.msg
        root.bind('<Control-c>', func=self.to_clip)

        # remove the outer frame if frame=False
        if not frame:
            root.overrideredirect(True)

        # default values for the buttons to return
        self.b1_return = ''
        self.b2_return = ''
        self.b3_return = ''
        self.b4_return = ''
        self.b5_return = ''

        # if b is a tuple unpack into the button text & return value
        if isinstance(b1, tuple):
            b1, self.b1_return = b1
        if isinstance(b2, tuple):
            b2, self.b2_return = b2
        if isinstance(b3, tuple):
            b3, self.b3_return = b3
        if isinstance(b4, tuple):
            b4, self.b4_return = b4
        if isinstance(b5, tuple):
            b5, self.b5_return = b5

        # main frame
        frm_1 = tkinter.Frame(root)
        frm_1.pack(ipadx=2, ipady=2)
        # the message
        message = tkinter.Label(frm_1, text=self.msg)
        message.pack(padx=8, pady=8)

        # if entry=True create and set focus
        if entry:
            self.entry = tkinter.Entry(frm_1)
            self.entry.pack()
            self.entry.focus_set()

        # button frame
        frm_2 = tkinter.Frame(frm_1)
        frm_2.pack(padx=4, pady=4)

        # buttons
        btn_1 = tkinter.Button(frm_2, width=8, text=b1)
        btn_1['command'] = self.b1_action
        btn_1.pack(side='left')
        if not entry:
            btn_1.focus_set()
        btn_2 = tkinter.Button(frm_2, width=8, text=b2)
        btn_2['command'] = self.b2_action
        btn_2.pack(side='left')
        btn_3 = tkinter.Button(frm_2, width=8, text=b3)
        btn_3['command'] = self.b3_action
        btn_3.pack(side='left')
        btn_4 = tkinter.Button(frm_2, width=10, text=b4)
        btn_4['command'] = self.b4_action
        btn_4.pack(side='left')
        btn_5 = tkinter.Button(frm_2, width=10, text=b5)
        btn_5['command'] = self.b5_action
        btn_5.pack(side='left')

        # the enter button will trigger the focused button's action
        btn_1.bind('<KeyPress-Return>', func=self.b1_action)
        btn_2.bind('<KeyPress-Return>', func=self.b2_action)
        btn_3.bind('<KeyPress-Return>', func=self.b3_action)
        btn_4.bind('<KeyPress-Return>', func=self.b4_action)
        btn_5.bind('<KeyPress-Return>', func=self.b5_action)

        # roughly center the box on screen
        # for accuracy see: https://stackoverflow.com/a/10018670/1217270
        root.update_idletasks()
        xp = (root.winfo_screenwidth() // 2) - (root.winfo_width() // 2)
        yp = (root.winfo_screenheight() // 2) - (root.winfo_height() // 2)
        geom = (root.winfo_width(), root.winfo_height(), xp, yp)
        root.geometry('{0}x{1}+{2}+{3}'.format(*geom))

        # call self.close_mod when the close button is pressed
        root.protocol("WM_DELETE_WINDOW", self.close_mod)

        # a trick to activate the window (on Windows 7)
        root.deiconify()

        # if t is specified: call time_out after t seconds
        if t:
            root.after(int(t*1000), func=self.time_out)

    def b1_action(self):
        try:
            x = self.entry.get()
        except AttributeError:
            self.returning = self.b1_return
            self.root.quit()
        else:
            if x:
                self.returning = x
                self.root.quit()


    def b2_action(self):
        self.returning = self.b2_return
        self.root.quit()

    def b3_action(self):
        self.returning = self.b3_return
        self.root.quit()

    def b4_action(self):
        self.returning = self.b4_return
        self.root.quit()

    def b5_action(self):
        self.returning = self.b5_return
        self.root.quit()

    # remove this function and the call to protocol
    # then the close button will act normally
    def close_mod(self):
        pass

    def time_out(self):
        try:
            x = self.entry.get()
        except AttributeError:
            self.returning = None
        else:
            self.returning = x
        finally:
            self.root.quit()

    def to_clip(self):
        self.root.clipboard_clear()
        self.root.clipboard_append(self.msg)
