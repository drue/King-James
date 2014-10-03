King James
==========

This software is for creating audio recorder appliances on Linux.  It
sports an HTML/JS interface for wireless remote control with a phone
or any other web browser.  The current target is a Raspberry Pi fitted
with the Wolfson audio card. 

The core consists of a Python Tornado server with a C++ module that
interfaces with ALSA and writes the FLAC files.  The recording core
is written to ensure dropout free performance.  Socket.IO, with
TornadIO2, is used for realtime communication with the controlling
browser.  The core sends levels and status back up to Tornado with
ZeroMQ over Unix sockets.

Features
--------

* reliable, direct to FLAC recording
* generous pre-roll buffer
* gapless stop and re-record 
* HTML/Javascript interface that works on iOS or Android
* good level meters with peak hold

![screenshot running in iOS simulator](https://github.com/drue/King-James/raw/master/screenshot.png "screenshot")

History
-------

This code is based on a project I started in 2001.  In 2012 the code
 was overhauled in a major way to build a new device.

The original device was a PC-104 SBC with a 266mHz Pentium-MMX, 4x20
serial LCD + keypad, PC-104>PCI adapter, a PDAUDIO-CF card, hard
drive, and power supply.


Status

* 2013-10-2 - custom BuildRoot for creating a small and fast booting
  image.  Boot time still needs optimization but it is much smaller and
  faster than Raspbian.

* 2014-04-20 - updating for Raspberry Pi with Wolfson card

* 2013-05-01 - Successfully field tested with a Pico ITX Atom and miniStreamer

* 2012-08-22 - Core is thoroughly tested.  Now working on finding an
  AP compatible wifi driver that doesn't crash the low latency kernel
  required for reading off USB without loss.

* 2012-01-17 - Basic software is done and ready for some hardware to
test on.  Still needs recipes and scripts for cross-compiling and
building disk images.  Also needs some configuration, file
renaming, better error handling, and maybe a Winky-Blinkie inspired
stereo phase meter.

* 2012-01-02 - King-James project begins again with ancient codebase.

