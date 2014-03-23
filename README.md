
xbee configuration utility
==========================

Using serial (series 1) Xbees for communicating between systems 
is convenient. Sometimes, I need to manage the configuration of 
these modules a little better than just manually. This utility 
will conveniently save and restore a number of parameters from 
an Xbee module connected to a serial port (which may be USB.)

Example configuration data:

    ATMY1
    ATIDF00
    ATDL0
    ATDH0
    ATCHF
    ATRO1
    ATRR1
    ATA10

Example command to save data to stdout:

    xbee -b 38400 -p /dev/ttyXbee0 dump

Exmple command to load data from file:

    xbee -b 38400 -p /dev/ttyXbee0 load somefile.txt

Defaults are 9600 baud and /dev/ttyUSB0.

The included code builds on modern Linux, and perhaps on other 
UNIX/POSIX-like systems.

Released under the MIT open source license.

