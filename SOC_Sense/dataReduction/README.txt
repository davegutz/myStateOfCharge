puTTY settings to set up to run ./puTTY_Forever
unless specified, use default
you can set or change these by breaking the batch process an opening puTTY using the normal startup icon
you can use middle mouse button to paste into running puTTY session

Session
	Serial line= "COM9"  (change this as session evolves)
	Connection type radio button= "Serial"
	Saved Sessions= "def"   (to be compatible with .puTTY_Forever command line usage
			Load
			Save
	Close window on exit radio button="Always"
Session - Logging
	Session logging radio button= "All session output"
	Log file name: C:\path\to\dataReduction\puttyLog&Y&M&D_&T.csv  (no quotes)
	What to do if the log file already exists radio button="Ask the user every time"
Terminal
	Set various terminal options checkbox="Implicit CR in every LF"
	Local echo radio button="Force on"
Window
	Columns="212"  Rows="32"
Window - Appearance
	Adjust the window border - Gap between text and window edge="3"
Window Selection
	Assign copy/paste actions to clipboards = all dropdowns = "System clipboard"
Window - Colours
	Default Foreground and Bold= "245 / 222 / 179"
	Default Background and Bold= "47 / 79 / 79"
Window - Fonts (if available) - Fonts used for ordinary text = "DejaVu Sans Mono 9"
Finally - DON'T FORGET!
Session
	Saved Sessions - Save