import json
from time import time

import math

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

class SocketIOHandler(RequestHandler):
    def get(self):
        self.render('socket.io.js')
        

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


class Status(SocketConnection):
    def on_open(self, info):
        print 'Ping', repr(info)

    def on_message(self, message):
        now = datetime.datetime.now()

        message['server'] = [now.hour, now.minute, now.second, now.microsecond / 1000]
        self.send(message)


class RouterConnection(SocketConnection):
    __endpoints__ = {'/peaks': Peaks,
                     '/status': Status}

    def on_open(self, info):
        print 'Router', repr(info)


SockRouter = TornadioRouter(RouterConnection)

application = Application(
    SockRouter.apply_routes([(r"/", IndexHandler),
                           (r"/socket.io.js", SocketIOHandler)]),
                           socket_io_port = 8000)


def zmq_producer(): 
    '''Produce a nice time series sine wave''' 
    socket = context.socket(zmq.PUB) 
    socket.bind('inproc://peaks')
    il = ioloop.IOLoop.instance()

    def doIt():
        x = time() 
        y = 2.5 * (1 + math.sin(x / 500)) 
        socket.send(json.dumps(dict(x=x, y=y))) 
        il.add_timeout(x + 0.25, doIt)
 
    doIt()

if __name__ == "__main__":
    ioloop.IOLoop.instance().add_callback(zmq_producer)
    socketio_server = SocketServer(application)

  
