#!/usr/bin/env python

from zmq.eventloop import ioloop
ioloop.install()

import os

from tornadio2 import SocketConnection, TornadioRouter, SocketServer
import tornado
from tornado.web import RequestHandler, Application, StaticFileHandler

from core import port

from commands import RecordHandler, ResetPeaksHandler
from status import Peaks, Status, Ping

class IndexHandler(RequestHandler):
    def get(self):
        self.render('index.html')


class RouterConnection(SocketConnection):
    __endpoints__ = {'/peaks': Peaks,
                     '/ping': Ping,
                     '/status': Status}

def main():

    try:
        SockRouter = TornadioRouter(RouterConnection)

        application = Application(SockRouter.apply_routes([(r"/", IndexHandler),
                                                             (r"/record", RecordHandler),
                                                             (r"/resetPeaks", ResetPeaksHandler),
                                                             (r"/flac/(.*)",tornado.web.StaticFileHandler, {"path": "."},)]),
                                   socket_io_port = 80,
                                   static_path = os.path.join(os.path.dirname(__file__), "static")
                                   )

        socketio_server = SocketServer(application)
    finally:
        port.stop()
        port.waitTillFinished()


if __name__=="__main__":
    main()
    
