import time
import os
import statvfs
import string

import const

last = 0
left = ""

def timeleft(ws=24, sr=96000, path='/home/drue/trax'):

    global last, left
    
    hourkb = (sr * (ws / 8) * 2 * 60 * 60 / 1024) / (1 / const.COMP_FACTOR)
    minutekb = (sr * (ws / 8) * 2 * 60 / 1024) / (1 / const.COMP_FACTOR)
    if time.time() - last > 10:
        x = os.statvfs(path)
	kb = x[statvfs.F_BFREE] * x[statvfs.F_BSIZE] / 1024
	hours = kb / hourkb
	minutes = (kb - (int(hours) * hourkb)) / minutekb
	last = time.time()
	left = "%02d:%02d:00" % (hours, minutes)
    return left


hoursecs = 60 * 60
minsecs = 60

def format_seconds(secs):
    if secs > hoursecs:
	hours = int(secs / hoursecs)
    else:
	hours = 0
    if secs > minsecs:
	x = secs - hours * hoursecs
	mins = int(x / minsecs)
    else:
	mins = 0
    secs = secs - (hours * hoursecs) - (mins * minsecs)
    return "%02d:%02d:%02d" % (hours, mins, secs)

