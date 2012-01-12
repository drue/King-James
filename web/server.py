#!/usr/bin/env python

import json
from time import time

import math
import os

import zmq
from zmq.eventloop import ioloop, zmqstream

ioloop.install()

from tornadio2 import SocketConnection, TornadioRouter, SocketServer
import tornado
from tornado.web import RequestHandler, Application
import datetime

context = zmq.Context() 
il = ioloop.IOLoop.instance()

STOPPED = 0
RECORDING = 1

class IndexHandler(RequestHandler):
    def get(self):
        self.render('index.html')


class Peaks(SocketConnection):
    def on_open(self, info):
        socket = context.socket(zmq.SUB)
        socket.connect('inproc://peaks')
        socket.setsockopt(zmq.SUBSCRIBE, '')
        stream = zmqstream.ZMQStream(socket, tornado.ioloop.IOLoop.instance())
        stream.on_recv(self.send)


class Status(SocketConnection):
    def on_open(self, info):
        socket = context.socket(zmq.SUB)
        socket.connect('inproc://status')
        socket.setsockopt(zmq.SUBSCRIBE, '')
        stream = zmqstream.ZMQStream(socket, tornado.ioloop.IOLoop.instance())
        stream.on_recv(self.send)


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


def peak_producer(): 
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
    def __init__(self):
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


if __name__ == "__main__":
    il.add_callback(peak_producer)
    il.add_callback(StatusProducer().start)
    socketio_server = SocketServer(application)

  
