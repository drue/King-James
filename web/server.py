#!/usr/bin/env python

import json
from time import time

import math
import os
import traceback

import zmq
from zmq.eventloop import ioloop, zmqstream
from zmq.devices.basedevice import ThreadDevice

ioloop.install()

from tornadio2 import SocketConnection, TornadioRouter, SocketServer
import tornado
from tornado.web import RequestHandler, Application
import datetime

from const import *

context = zmq.Context() 
il = ioloop.IOLoop.instance()

## bind a forwarder here so multiple spools can connect without conflict
pr = ThreadDevice(zmq.FORWARDER, zmq.SUB, zmq.PUB)
pr.bind_in('ipc:///tmp/progressIn.ipc')
pr.bind_out('ipc:///tmp/progressOut.ipc')
pr.setsockopt_in(zmq.SUBSCRIBE, '')
pr.start()

mode = STOPPED
n = 0

import transport
port = transport.newTPort(1, 24, 48000)
try:
    class IndexHandler(RequestHandler):
        def get(self):
            self.render('index.html')

    class RecordHandler(RequestHandler):
        def newFile(self):
            global n
            n+= 1
            return "recording%s.flac" % n

        def post(self):
            global mode
            if mode == STOPPED:
                port.startRecording(self.newFile())
                mode = RECORDING
            else:
                port.stopRecording()
                mode = STOPPED

    class Peaks(SocketConnection):
        def on_open(self, info):
            self.socket = context.socket(zmq.SUB)
            self.socket.connect('ipc:///tmp/peaks.ipc')
            self.socket.setsockopt(zmq.SUBSCRIBE, '')
            stream = zmqstream.ZMQStream(self.socket, tornado.ioloop.IOLoop.instance())
            stream.on_recv(self.send)

    class Status(SocketConnection):
        def on_open(self, info):
            self.socket = context.socket(zmq.SUB)
            self.socket.connect('ipc:///tmp/progressOut.ipc')
            self.socket.setsockopt(zmq.SUBSCRIBE, '')
            self.stream = zmqstream.ZMQStream(self.socket, tornado.ioloop.IOLoop.instance())
            self.stream.on_recv(self.process)

        def on_close(self):
            self.stream.close()
            self.socket.close()
            
        def process(self, data):
            o = []
            for msg in data:
                d = json.loads(msg)
                d['c'] = os.getloadavg()
                d['r'] = 0
                o.append(json.dumps(d))
            if o:
                self.send(o)

    class Ping(SocketConnection):
        def on_open(self, info):
            print 'Ping', repr(info)

        def on_message(self, message):
            now = datetime.datetime.now()

            message['server'] = [now.hour, now.minute, now.second, now.microsecond / 1000]
            self.send(message)


    class RouterConnection(SocketConnection):
        __endpoints__ = {'/peaks': Peaks,
                         '/ping': Ping,
                         '/status': Status}


    SockRouter = TornadioRouter(RouterConnection)

    application = Application(SockRouter.apply_routes([(r"/", IndexHandler),
                                                         (r"/record", RecordHandler)]),
                               socket_io_port = 8000,
                               static_path = os.path.join(os.path.dirname(__file__), "static")
                               )

    if __name__ == "__main__":
        socketio_server = SocketServer(application)
finally:
    port.stop()
    port.waitTillFinished()


