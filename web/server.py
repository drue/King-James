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


class IndexHandler(RequestHandler):
    def get(self):
        self.render('index.html')


class Peaks(SocketConnection):
    def on_open(self, info):
        print '>>> Peaks', repr(info)

        socket = context.socket(zmq.SUB)
        socket.connect('inproc://peaks')
        socket.setsockopt(zmq.SUBSCRIBE, '')
        stream = zmqstream.ZMQStream(socket, tornado.ioloop.IOLoop.instance())
        stream.on_recv(self.send)

    def on_message(self, message):
        print ">>> got a message on the peaks chan", message

    def on_close(self):
        print ">>> peaks closed"


class Ping(SocketConnection):
    def on_open(self, info):
        print 'Ping', repr(info)

    def on_message(self, message):
        now = datetime.datetime.now()

        message['server'] = [now.hour, now.minute, now.second, now.microsecond / 1000]
        self.send(message)


class RouterConnection(SocketConnection):
    __endpoints__ = {'/peaks': Peaks,
                     '/ping': Ping}


SockRouter = TornadioRouter(RouterConnection)

application = Application(SockRouter.apply_routes([(r"/", IndexHandler)]),
                           socket_io_port = 8000,
                           static_path = os.path.join(os.path.dirname(__file__), "static")
                           )


def zmq_producer(): 

    socket = context.socket(zmq.PUB) 
    socket.bind('inproc://peaks')
    il = ioloop.IOLoop.instance()

    def peaks(file):
        f = open(file, "r")
        lines = f.readlines()
        while True:
            for line in lines:
                yield line
    p = peaks("peakz")

    def doIt():
        socket.send(p.next()) 
        il.add_timeout(time() + 0.10, doIt)
 
    doIt()

if __name__ == "__main__":
    ioloop.IOLoop.instance().add_callback(zmq_producer)
    socketio_server = SocketServer(application)

  
