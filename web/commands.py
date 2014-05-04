from datetime import datetime
from os.path import join

from tornado.web import RequestHandler

import core
from const import *
from status import resetBTimer


class RecordHandler(RequestHandler):

    def post(self):
        global mode
        if core.mode == STOPPED:
            core.start()
            core.mode = RECORDING
        else:
            core.port.stopRecording()
            core.mode = STOPPED

class ResetPeaksHandler(RequestHandler):
    def post(self):
        core.port.resetPeaks()

class ResetBTimerHandler(RequestHandler):
    def post(self):
        resetBTimer()
