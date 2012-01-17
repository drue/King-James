#!/usr/bin/env python

from zmq.eventloop import ioloop
ioloop.install()

import os

from tornadio2 import SocketConnection, TornadioRouter, SocketServer
import tornado
from tornado.web import RequestHandler, Application

from core import port

from commands import RecordHandler, ResetPeaksHandler
from status import Peaks, Status, Ping

try:
    class IndexHandler(RequestHandler):
        def get(self):
            self.render('index.html')


    class RouterConnection(SocketConnection):
        __endpoints__ = {'/peaks': Peaks,
                         '/ping': Ping,
                         '/status': Status}


    SockRouter = TornadioRouter(RouterConnection)

    application = Application(SockRouter.apply_routes([(r"/", IndexHandler),
                                                         (r"/record", RecordHandler),
                                                         (r"/resetPeaks", ResetPeaksHandler)]),
                               socket_io_port = 8000,
                               static_path = os.path.join(os.path.dirname(__file__), "static")
                               )

    if __name__ == "__main__":
        socketio_server = SocketServer(application)
finally:
    port.stop()
    port.waitTillFinished()


