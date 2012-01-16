from time import time
from zmq.eventloop import ioloop
import json
import os
import zmq

from const import *

il = ioloop.IOLoop.instance()

def peak_producer(context): 
    """ some canned peak data for testing the UI """
    socket = context.socket(zmq.PUB) 
    socket.bind('inproc://peaks')
    

    def peaks(file):
        f = open(file, "r")
        while True:
            for line in f:
                yield line
            f.seek(0)
    p = peaks("peakz")

    def doIt():
        socket.send(p.next()) 
        il.add_timeout(time() + 0.10, doIt)
 
    doIt()


class StatusProducer(object):
    """ some canned status data for testing the UI """
    def __init__(self, context):
        self.socket = context.socket(zmq.PUB) 
        self.socket.bind('inproc://status')

    def start(self):
        self.t = time()
        il.add_callback(self.doIt)

    def doIt(self):
        status = {'t' : time() - self.t,
                  'r' : 3600 * 8.3,
                  'm' : RECORDING,
                  'c' : os.getloadavg(),
                  'b' : 60
                  }
        self.socket.send(json.dumps(status))
        il.add_timeout(time() + 0.25, self.doIt)


