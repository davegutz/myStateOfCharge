# TestSOC.py Script to Interface to puTTY Serial Monitor

The Particle devices use the Serial interface to stream text-based data files.   This is done real-time with accurate time stamps allowing very accurate debugging.   I perform debugging by overplotting the results with simulated results using the same sampled data inputs.

The user starts [TestSOC.py](../py/TestSOC.py).   Either in the system environment or virtual environment (venv) call Python3.10.10 (lower may work) using the following imports:

```
 python -m pip install --upgrade pip
 python -m pip install configparser
 python -m pip install psutil
 python -m pip install pyperclip
 python -m pip install reportlab
 python -m pip install matplotlib
 python -m pip install easygui
```

Start TestSOC.py.   A gui like this should appear.
TODO:  ...
