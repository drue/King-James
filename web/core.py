import os

os.system("alsactl restore")

import sys

import const
from config import config, save_config
import ConfigParser
import os
import subprocess

mode = const.STOPPED

depth = 24
rate = 48000
channels = 2
card = "0"
#card = "1"
comp_ratio = .7

if not sys.DEV_MODE:
    import transport
    port = transport.newTPort(card, depth, rate)
else:
    from devport import DevPort
    port = DevPort(card, depth, rate)
    
LOC="/var/audio"

def currentFilename(set=False):    
    try:
        n = int(config.get('DEFAULT', 'nfile'))
    except ConfigParser.NoOptionError, ValueError:
        n = 0

    if set:
        n += 1
        config.set('DEFAULT', 'nfile', str(n))
        save_config()
    return "recording-%05d.flac" % n

def start():
    global mode
    if mode == const.STOPPED:
        path = os.path.join(LOC, currentFilename(set=True))
        port.startRecording(path)
        mode = const.RECORDING

def stop():
    global mode
    if mode == const.RECORDING:
        fname = currentFilename()
        port.stopRecording()
        #    sync_dir()
        mode = const.STOPPED

def sync_dir():
    d = os.open(LOC, os.O_RDONLY | os.O_DIRECTORY)
    os.fsync(d)
    os.close(d)

