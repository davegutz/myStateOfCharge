import easygui, os
data_file = easygui.fileopenbox(msg="choose your data file to plot")
data_file_old_txt = easygui.fileopenbox(msg="pick new file name, return to keep", title="get new file name")
if data_file_old_txt is None:
    data_file_old_txt = data_file
if data_file_old_txt != '' and len(data_file_old_txt) > 3:
    os.rename(data_file, data_file_old_txt)
unit_key = easygui.enterbox(msg="enter soc0p, soc1a, soc0a, or soc1a", title="get unit_key", default="soc1a")
print("data_file_old_txt ='", data_file_old_txt, "'; unit_key =", unit_key)
