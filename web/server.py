#!/usr/bin/env python

import sys

try:
    sys.argv.index("DEBUG")
except ValueError:
    sys.DEV_MODE = False
else:
    sys.DEV_MODE = True

from zmq.eventloop import ioloop
ioloop.install()

import os

from sockjs.tornado import SockJSRouter, SockJSConnection
import tornado
from tornado.web import RequestHandler, Application, StaticFileHandler


from commands import RecordHandler, ResetPeaksHandler, ResetBTimerHandler
from status import Status

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
        self.write(open('index.html').read())


def main():
    import core

    # turn on heavy debug logging
    #setup_logging()

    try:
        SockRouter = SockJSRouter(Status)

        application = Application(SockRouter.apply_routes([(r"/", IndexHandler),
                                                           (r"/record", RecordHandler),
                                                           (r"/resetPeaks", ResetPeaksHandler),
                                                           (r"/resetBTimer", ResetBTimerHandler),
                                                           (r"/flac/(.*)",tornado.web.StaticFileHandler, {"path": "."},)]),
                                                           static_path = os.path.join(os.path.dirname(__file__), "static"),
                                                           debug = sys.DEV_MODE
            )
        

        if not sys.DEV_MODE:
            port = 80
        else:
            port = 8080

        application.listen(port)
        ioloop.IOLoop.instance().start()
        
    finally:
        core.port.stop()
        core.port.waitTillFinished()


if __name__=="__main__":
    main()
