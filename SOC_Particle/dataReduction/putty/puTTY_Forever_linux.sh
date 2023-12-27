#!/bin/bash
# install this in /usr/local/bin with root priveledges
# sudo cp puTTY_Forever_linux.sh /usr/local/bin
# sudo chmod +777 /usr/local/bin/puTTY_Forever_linux.sh
# at cli start putty; make def config - see puTTY_Windows_setup_def.odt
while true
do
	nice --20 putty -load "def"
	sleep 5
done

