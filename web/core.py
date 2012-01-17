import transport
from const import *


mode = STOPPED

depth = 24
rate = 48000
card = 1
comp_ratio = .65


port = transport.newTPort(card, depth, rate)
