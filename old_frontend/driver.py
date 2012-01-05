import sys
sys.path.append("/home/drue/james/core/python")

import time
import os
from select import select, error
import socket
import const

import lcd
import menus

class Driver(object):
    def __init__(self):
	self.lr = open("/dev/ttyS0", "r")
	self.lw = open("/dev/ttyS0", "w")
	self.screen = lcd.LCDWriter(self.lw)
	self.screen.clear()
	self.light = 1
        self.screen._sendCommand(lcd.SET_FONT, 1)
        self.screen._sendCommand(lcd.SET_FONT_METRICS, 1,1,1,1,0)
	self.state = menus.StoppedState()
	self.state.entered_state(self)

        
## this needs shutdown and error handling, etc...	
    def main_event_loop(self):
	while(1):
            try:
                a, b, c = select([self.lr],[],[], .1)
            except error, e:
                print dir(e)
            else:
                if self.lr in a:
                    key = self.lr.read(1)
                    self.state.key_down(key, self)
            self.state.pulse(self)

class DummyDriver(Driver):
    # brain dead named pipe driver for development without keypad
    def __init__(self):
        self.cons = []
        self.lr = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.cons.append(self.lr)
        try:
            os.unlink("/tmp/jamessock")
        except OSError, n:
            if n.errno == 2:
                pass
            else:
                raise OSError, n
        self.lr.bind("/tmp/jamessock")
        self.lr.listen(1)
        self.screen = lcd.DummyWriter(0)
	self.screen.clear()
	self.screen.write("Ready...")
	self.light = 1
	self.state = menus.StoppedState()
	self.state.entered_state(self)
        
    def main_event_loop(self):
	while(1):
	    a, b, c = select(self.cons,[],[], 1)
	    if self.lr in a:
                s = self.lr.accept()[0]
                self.cons.append(s)
            for s in a:
                if s != self.lr:
                    key = s.recv(1)
                    self.state.key_down(key, self)
	    self.state.pulse(self)

            
        

if __name__ == "__main__":
    if len(sys.argv) == 1:
        import transport
        # No arguments, start in daemon mode
        d = Driver()
        d.main_event_loop()
    elif len(sys.argv) == 2 and sys.argv[1]=="-d":
        const.DEBUG = 1
        d = DummyDriver()
        d.main_event_loop()
    """
    else:
        import transport
        # command line recording: core <bps> <sr> <file name> <secs>
        t = transport.newTPort(int(sys.argv[1]), int(sys.argv[2]))
        t.startRecording(sys.argv[3])
        print "Recording started, %s/%s to file %s..." % (sys.argv[1],sys.argv[2],sys.argv[3] )
        try:
            sleep = int(sys.argv[4])
        except IndexError:
            sleep = 15
        try:
            time.sleep(sleep)
        except KeyboardInterrupt:
            print "Interrupted..."
        t.stop()
        t.waitTillFinished()
        print "Done!"
    """
