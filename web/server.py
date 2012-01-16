#!/usr/bin/env python

import json
from time import time

import math
import os
import traceback

import zmq
from zmq.eventloop import ioloop, zmqstream

ioloop.install()

from tornadio2 import SocketConnection, TornadioRouter, SocketServer
import tornado
from tornado.web import RequestHandler, Application
import datetime

context = zmq.Context() 
il = ioloop.IOLoop.instance()

import transport
port = transport.newTPort(1, 24, 48000)

class IndexHandler(RequestHandler):
    def get(self):
        self.render('index.html')


class Peaks(SocketConnection):
    def on_open(self, info):
        socket = context.socket(zmq.SUB)
        socket.connect('ipc:///tmp/peaks.ipc')
        socket.setsockopt(zmq.SUBSCRIBE, '')
        stream = zmqstream.ZMQStream(socket, tornado.ioloop.IOLoop.instance())
        stream.on_recv(self.send)

class Status(SocketConnection):
    def on_open(self, info):
        socket = context.socket(zmq.SUB)
        socket.connect('ipc:///tmp/progress.ipc')
        socket.setsockopt(zmq.SUBSCRIBE, '')
        stream = zmqstream.ZMQStream(socket, tornado.ioloop.IOLoop.instance())
        stream.on_recv(self.process)

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

application = Application(SockRouter.apply_routes([(r"/", IndexHandler)]),
                           socket_io_port = 8000,
                           static_path = os.path.join(os.path.dirname(__file__), "static")
                           )

if __name__ == "__main__":

    try:
        socketio_server = SocketServer(application)
    finally:
        port.stop()
        port.waitTillFinished()

  
