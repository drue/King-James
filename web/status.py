import os
from time import time
import json
import datetime

import tornado
from tornadio2 import SocketConnection
import zmq
from zmq.eventloop.zmqstream import ZMQStream
from zmq.devices.basedevice import ThreadDevice

import core

context = zmq.Context() 

## bind a forwarder here so multiple spools can connect without conflict
pr = ThreadDevice(zmq.FORWARDER, zmq.SUB, zmq.PUB)
pr.bind_in('ipc:///tmp/progressIn.ipc')
pr.bind_out('ipc:///tmp/progressOut.ipc')
pr.setsockopt_in(zmq.SUBSCRIBE, '')
pr.start()

class Status(SocketConnection):
    remaining = 0
    rTime = 0
    def on_open(self, info):
        self.socket = context.socket(zmq.SUB)
        self.socket.connect('ipc:///tmp/progressOut.ipc')
        self.socket.setsockopt(zmq.SUBSCRIBE, '')
        self.stream = ZMQStream(self.socket, tornado.ioloop.IOLoop.instance())
        self.stream.on_recv(self.process)

    def on_close(self):
        self.stream.close()
        self.socket.close()

    def process(self, data):
        o = []
        if (time() - Status.rTime > 30):
            stat = os.statvfs('.')
            Status.remaining = (stat.f_bsize * stat.f_bavail) / ((core.depth / 8) * core.rate * core.comp_ratio)
            Status.rTime = time()

        for msg in data:
            d = json.loads(msg)
            d['c'] = os.getloadavg()
            d['r'] = Status.remaining
            d['s'] = core.port.gotSignal()
            d['f'] = "%s/%s" % (core.depth, str(core.rate)[:2])
            o.append(json.dumps(d))
        if o:
            self.send(o)


class Peaks(SocketConnection):
    def on_open(self, info):
        self.socket = context.socket(zmq.SUB)
        self.socket.connect('ipc:///tmp/peaks.ipc')
        self.socket.setsockopt(zmq.SUBSCRIBE, '')
        stream = ZMQStream(self.socket, tornado.ioloop.IOLoop.instance())
        stream.on_recv(self.send)


class Ping(SocketConnection):
    def on_message(self, message):
        now = datetime.datetime.now()

        message['server'] = [now.hour, now.minute, now.second, now.microsecond / 1000]
        self.send(message)
