import os

os.system("alsactl restore")

import transport
from const import *


mode = STOPPED

depth = 24
rate = 48000
card = "miniStreamer"
#card = "1"
comp_ratio = .65


port = transport.newTPort(card, depth, rate)
