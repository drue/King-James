from time import time
import json
from tornado.ioloop import IOLoop
import zmq
from zmq.eventloop.zmqstream import ZMQStream

class DevPort(object):
    def __init__(self, card, depth, rate):
        self.card = card
        self.depth = depth
        self.rate = rate
        self.running = False

        context = zmq.Context() 

        self.prog_socket = context.socket(zmq.PUB)
        self.prog_socket.connect('ipc:///tmp/progressIn.ipc')
        self.prog_stream = ZMQStream(self.prog_socket, IOLoop.instance())
        self.scheduleProgress()
        
        self.peak_socket = context.socket(zmq.PUB)
        self.peak_socket.bind('ipc:///tmp/peaks.ipc')
        self.peak_stream = ZMQStream(self.peak_socket, IOLoop.instance())
        self.sendPeaks()
        
    def startRecording(self, path):
        self.curPath = path
        self.started = time()
        self.running = True

    def stopRecording(self):
        self.running = False

    def resetPeaks(self):
        pass

    def stop(self):
        self.stopRecording()

    def  waitTillFinished(self):
        pass

    def gotSignal(self):
        return True

    def sendProgress(self):
        duration = 0
        mode = 0
        if self.running:
            duration = int((time() - self.started) * self.rate)
            mode = 1

        d = {'t':duration, 'm': mode, 'b' : 0 }

        s = json.dumps(d)
        self.prog_stream.send(s)        
        IOLoop.instance().add_timeout(time() + .333, self.sendProgress)

    def sendPeaks(self):
        l = [-140, -140, -140, -140]
        self.peak_stream.send(json.dumps(l))
        IOLoop.instance().add_timeout(time() + .1, self.sendPeaks)
        
    def scheduleProgress(self):
        IOLoop.instance().add_timeout(time() + .333, self.sendProgress)
