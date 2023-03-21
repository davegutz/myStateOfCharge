::Windows bat file
:: run with > ./puTTY_Forever.bat
:loop
putty -load "def"
timeout /t 2
goto loop
