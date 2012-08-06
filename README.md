King James
==========

This software is for creating audio recorder appliances on Linux.  The
main use case is concert recording with a small SBC such as a
RaspberryPi or BeagleBone.  It sports an HTML/JS interface for
wireless remote control with a phone or any other web browser.

The core consists of a Python Tornado server with a C++ module that
interfaces with ALSA and writes the FLAC files.  Socket.IO, with
TornadIO2, is used for realtime communication with the controlling
browser.  Components are tied together with ZeroMQ.

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

This code is based on a project I started in 2001.  In 2012 the code is
being overhauled in a major way to build a new device.

The original device was a PC-104 SBC with a 266mHz Pentium-MMX, 4x20
serial LCD + keypad, PC-104>PCI adapter, a SPDIF/AES sound card, hard
drive, and power supply.  When the PDAUDIO-CF was released I switched
to that.  I used that recorder for a few years but eventually I
switched to a commercial handheld because it was much smaller and
lighter.

Now with SBCs like Raspberry-Pi and BeagleBone, I can make a recorder 
that is better than commercial units for my purposes.


Status
------

* 2012-01-17 - Basic software is done and ready for some hardware to
test on.  Still needs recipes and scripts for cross-compiling and
building disk images.  Also needs some configuration, file
renaming, better error handling, and maybe a Winky-Blinkie inspired
stereo phase meter.

* 2012-01-02 - King-James project begins again with ancient codebase.
