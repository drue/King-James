King James
==========

This software is for creating audio recorder appliances on Linux.  The
main use case is concert recording with a small SBC such as a
RaspberryPi or BeagleBone.  It is controlled with an HTML/JS
interface.

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

The handheld commercial units available today all have issues.  Only
two have digital inputs.  One is a piece of crap and the other
wastes space and budget on built-in mics that I'm not too keen on.
I also don't really need analog inputs though sometimes they do come
in handy.

Now with SBCs like Raspberry-Pi and BeagleBone, I can make a better
recorder for my purposes without compromising on size or price.  I
particularly want remote control via my phone.


Status
------

* 2012-01-17 - Basic software is done and ready for some hardware to
test on.  Still needs recipes and scripts for cross-compiling and
building disk images.  Also needs some configuration, file
renaming, better error handling, and maybe a Winky-Blinkie inspired
stereo phase meter.

* 2012-01-02 - King-James project begins again with ancient codebase.
