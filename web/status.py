from math import floor
from time import time
import datetime
import json
import os
from subprocess import check_call

import tornado
from tornadio2 import SocketConnection
import zmq
from zmq.eventloop.zmqstream import ZMQStream
from zmq.devices.basedevice import ThreadDevice

import core
from config import config, save_config

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
    pTime = 0
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
        now = time()
        if (now - Status.rTime > 30):
            stat = os.statvfs('.')
            Status.remaining = (stat.f_bsize * stat.f_bavail) / ((core.depth / 8) * core.channels * core.rate * core.comp_ratio)
            Status.rTime = now
            updateBTimer()
            save_config()

        if (now - Status.pTime > 0.5):
            for msg in data:
                d = json.loads(msg)
                d['c'] = os.getloadavg()
                d['r'] = Status.remaining
                d['s'] = core.port.gotSignal()
                d['f'] = "%s/%s" % (core.depth, str(core.rate)[:2])
                #                d['bt'] = hrBTimer()
                #                d['ct'] = getTemp()
                o.append(json.dumps(d))
            if o:
                self.send(o)
                #                print(o)
            Status.pTime = now


class Peaks(SocketConnection):
    def on_open(self, info):
        self.socket = context.socket(zmq.SUB)
        self.socket.connect('ipc:///tmp/peaks.ipc')
        self.socket.setsockopt(zmq.SUBSCRIBE, '')
        stream = ZMQStream(self.socket, tornado.ioloop.IOLoop.instance())
        stream.on_recv(self.do_send)

    def do_send(self, *args, **kwargs):
        #        print(">>> peakz %s %s" % (args, kwargs))
        self.send(*args, **kwargs)


class Ping(SocketConnection):
    def on_message(self, message):
        now = datetime.datetime.now()

        message['server'] = [now.hour, now.minute, now.second, now.microsecond / 1000]
        self.send(message)


lastUpdate = time()

def resetBTimer():
    config['btimer'] = 0
    pass

def updateBTimer():
    global lastUpdate
    now = time()
    delta = now - lastUpdate
    lastUpdate = now
    config.set('DEFAULT', 'btimer', delta + float(config.get('DEFAULT', 'btimer', True)))

def hrBTimer():
    h = floor(config.get('DEFAULT', 'btimer', True) / 60.0)
    m = config.get('DEFAULT','btimer', True) % 60
    return "%d:%02d" % (h, m)

theTemp = "0"

def updateTemp():
    o = check_call("sensors", shell='true')
    t = o.search.group(n)
    global theTemp
    theTemp = t

def getTemp():
    return theTemp
