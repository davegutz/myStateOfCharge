from tkinter import Tk
import os
import pyperclip


def addToClipBoard(text):
    pyperclip.copy(text)


r = Tk()
r.withdraw()
r.clipboard_clear()
r.clipboard_append('Has clipboard?')
r.update()  # now it stays on the clipboard after the window is closed
print('on clipboard:', r.clipboard_get())
result = r.selection_get(selection="CLIPBOARD")
print('result:', result)
addToClipBoard(result)
r.destroy()
