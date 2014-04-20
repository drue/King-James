import os

os.system("alsactl restore")

import transport
from const import *

from datetime import datetime


mode = STOPPED

depth = 24
rate = 48000
channels = 2
card = "0"
#card = "1"
comp_ratio = .7


port = transport.newTPort(card, depth, rate)

def start():
    port.startRecording("/var/audio/%s.flac" % datetime.now().isoformat())
