import os

os.system("alsactl restore")

import transport
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


port = transport.newTPort(card, depth, rate)

LOC="/var/audio"

def start():

    try:
        n = int(config.get('DEFAULT', 'nfile'))
    except ConfigParser.NoOptionError, ValueError:
        n = 0
    n += 1
    
    config.set('DEFAULT', 'nfile', str(n))
    save_config()
    
    path = os.path.join(LOC, "recording-%05d.flac" % n)

    port.startRecording(path)
    mode = const.RECORDING

def stop():
    port.stopRecording()
    #    sync_dir()
    mode = const.STOPPED

def sync_dir():
    d = os.open(LOC, os.O_RDONLY | os.O_DIRECTORY)
    os.fsync(d)
    os.close(d)

