#!/usr/bin/env python

from zmq.eventloop import ioloop
ioloop.install()

import os

from tornadio2 import SocketConnection, TornadioRouter, SocketServer
import tornado
from tornado.web import RequestHandler, Application, StaticFileHandler

import core

from commands import RecordHandler, ResetPeaksHandler, ResetBTimerHandler
from status import Status

import sys
import logging


FORMAT = "%(asctime)-15s : %(levelname)s :  %(message)s"
LOG_FILE = '/var/log/james.log'

def setup_logging(level=logging.DEBUG):
    logging.basicConfig(level=level,
                    format=FORMAT,
                    filename=LOG_FILE,
                    filemode='a')

    root_logger = logging.getLogger('')
    strm_out = logging.StreamHandler(sys.stdout)
    root_logger.setLevel(logging.DEBUG)
    root_logger.addHandler(strm_out)


class IndexHandler(RequestHandler):
    def get(self):
        self.render('index.html')


def main():
    # turn on heavy debug logging
    #setup_logging()

    try:
        SockRouter = TornadioRouter(Status)

        application = Application(SockRouter.apply_routes([(r"/", IndexHandler),
                                                             (r"/record", RecordHandler),
                                                             (r"/resetPeaks", ResetPeaksHandler),
                                                             (r"/resetBTimer", ResetBTimerHandler),
                                                             (r"/flac/(.*)",tornado.web.StaticFileHandler, {"path": "."},)]),
                                   socket_io_port = 80,
                                   static_path = os.path.join(os.path.dirname(__file__), "static")
                                   )

        socketio_server = SocketServer(application)
    finally:
        core.port.stop()
        core.port.waitTillFinished()


if __name__=="__main__":
    main()
    
