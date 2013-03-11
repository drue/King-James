from datetime import datetime
from os.path import join

from tornado.web import RequestHandler

import core
from const import *

n = 0

LOC="/var/audio"

class RecordHandler(RequestHandler):
    def newFile(self):
        global n
        n+= 1
        return join(LOC, "%s.flac" % datetime.now().isoformat())

    def post(self):
        global mode
        if core.mode == STOPPED:
            core.port.startRecording(self.newFile())
            core.mode = RECORDING
        else:
            core.port.stopRecording()
            core.mode = STOPPED

class ResetPeaksHandler(RequestHandler):
    def post(self):
        core.port.resetPeaks()

