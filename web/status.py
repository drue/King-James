from math import floor
from time import time
import datetime
import json
import os
from subprocess import check_call

import tornado
from sockjs.tornado import SockJSConnection
import zmq
from zmq.eventloop.zmqstream import ZMQStream
from zmq.devices.basedevice import ThreadDevice
from ConfigParser import NoOptionError

import core
from config import config, save_config
import commands
import sys

context = zmq.Context() 

## bind a forwarder here so multiple spools can connect without conflict
pr = ThreadDevice(zmq.FORWARDER, zmq.SUB, zmq.PUB)
pr.bind_in('ipc:///tmp/progressIn.ipc')
pr.bind_out('ipc:///tmp/progressOut.ipc')
pr.setsockopt_in(zmq.SUBSCRIBE, '')
pr.start()

class Status(SockJSConnection):
    remaining = 0
    rTime = 0
    pTime = 0
    def on_open(self, info):
        self.prog_socket = context.socket(zmq.SUB)
        self.prog_socket.connect('ipc:///tmp/progressOut.ipc')
        self.prog_socket.setsockopt(zmq.SUBSCRIBE, '')
        self.prog_stream = ZMQStream(self.prog_socket, tornado.ioloop.IOLoop.instance())
        self.prog_stream.on_recv(self.process_prog)

        self.peak_socket = context.socket(zmq.SUB)
        self.peak_socket.connect('ipc:///tmp/peaks.ipc')
        self.peak_socket.setsockopt(zmq.SUBSCRIBE, '')        
        self.peak_stream = ZMQStream(self.peak_socket, tornado.ioloop.IOLoop.instance())
        self.peak_stream.on_recv(self.process_peaks)
        
    def on_close(self):
        self.prog_stream.close()
        self.prog_socket.close()

        self.peak_stream.close()
        self.peak_socket.close()
        
    def process_prog(self, data):
        now = time()
        
        if (now - Status.rTime > 30):
            if sys.DEV_MODE:
                stat = os.statvfs('.')
            else:
                stat = os.statvfs('/var/audio')
            Status.remaining = (stat.f_bsize * stat.f_bavail) / ((core.depth / 8) * core.channels * core.rate * core.comp_ratio)
            Status.rTime = now
            updateBTimer()
            updateTemp()
            save_config()
            #            core.sync_dir()

        for msg in data:
            d = json.loads(msg)
            d['_t'] = 'status'
            # d['c'] = os.getloadavg()
            d['r'] = Status.remaining
            d['s'] = core.port.gotSignal()
            d['ss'] = core.depth
            d['sr'] = core.rate
            #                d['bt'] = hrBTimer()
            d['ct'] = int(getTemp())
            #            if o:
            self.send(d)
        Status.pTime = now

    def process_peaks(self, data):
        d = {'_t': "peaks", "p":json.loads(data[0])}
        self.send(d)

    def on_message(self, message):
        # for handling ping
        now = datetime.datetime.now()

        message = json.loads(message)

        message['server'] = [now.hour, now.minute, now.second, now.microsecond / 1000]
        message['_t'] = "pong"
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
    try:
        saved = float(config.get('DEFAULT', 'btimer', True))
    except NoOptionError:
        saved = 0
    config.set('DEFAULT', 'btimer', delta + saved)

def hrBTimer():
    h = floor(config.get('DEFAULT', 'btimer' / 60.0))
    m = config.get('DEFAULT','btimer') % 60
    return "%d:%02d" % (h, m)

theTemp = "0"

def updateTemp():
    if sys.DEV_MODE:
        return 0
    
    with open( "/sys/class/thermal/thermal_zone0/temp" ) as tempfile:
        cpu_temp = tempfile.read()
    global theTemp
    theTemp = float(cpu_temp)/1000 * 1.8 + 32


def getTemp():
    return theTemp
