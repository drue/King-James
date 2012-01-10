from state import *
from math import log
import sys

class StoppedState(BaseState):
    def __init__(self):
	BaseState.__init__(self)
	self.rec_state = RecordingState(self)
	self.menu_state = MainMenu(self)
	self.rec_state.menu_state = self.menu_state

    def handle_rec(self, sender):
	sender.state = self.rec_state
	sender.state.startRecording(sender)
	sender.state.entered_state(sender)

    def handle_menu(self, sender):
	sender.state = self.menu_state
	sender.state.last_state = self
	sender.state.entered_state(sender)

    def entered_state(self, sender):
	BaseState.entered_state(self, sender)	
	s = sender.screen
	s.clear()

        sr = `config.sr`[:2]
        ws = config.ws

	s.setInsert(STATUS)
	s.write('STOP  %s/%s 00:00:00' % (ws,sr))
        self.drawTime()
        
    def pulse(self, sender):
        self.drawTime()


class RecordingState(BaseState):
    def __init__(self, stop_state):
	BaseState.__init__(self)
	self.stop_state = stop_state
        self.flash = 0
        self.t = transport.newTPort(config.ws, config.sr)
        self._time = self._elapsed = ''
        self._peaks = (0,0)
        self._lastPeak = 0
        
    def handle_stop(self, sender):
	self.t.stop()
	self.t.waitTillFinished()
	#del(sender.t)
	sender.state = self.stop_state
	sender.state.entered_state(sender)

    def handle_menu(self, sender):
	sender.state = self.menu_state
	sender.state.last_state = self
	sender.state.entered_state(sender)
	
    def handle_rec(self, sender):
        self.t.resetPeaks()

    def entered_state(self, sender):
	BaseState.entered_state(self, sender)
	sender.screen.clear()
	self.redraw_screen()

    def redraw_screen(self):
	self.screen.setInsert(STATUS)
        self.screen.write("REC   %s/%s " % (self.cur_ws,
                                            `self.cur_sr`[:2]) + tools.format_seconds(time.time() - self.rec_started))
	self.screen.setInsert(TIME)
	t = time.localtime()
	self.screen.write('%02d:%02d   REM %s' % (t[3], t[4], tools.timeleft(sr=self.cur_sr, ws=self.cur_ws)))
        self.drawPeaks()
        
    def drawPeaks(self):
        t = time()
        if t - self._lastPeaks < 1:
            return

        self._lastPeaks = t
        
        if self.cur_ws == 16:
            x = MAX16
        else:
            x = MAX24
        a,b = self.t.getPeaks()

        if a == 0:
            self.screen.setInsert(PEAKL)
            self.screen.write('--')
        else:
            try:
                l = round(abs(20 * log ((a/x)) / log(10.0)))
            except OverflowError:
                l = 0.0


            self.screen.setInsert(PEAKL)
            if l >= 10.0:
                self.screen.write(`l`[:2])
            elif l >= 1.0:
                self.screen.write(' ' + `l`[:1])            
            else:
                self.screen.write(`l`[1:2])
        if b == 0:
            self.screen.setInsert(PEAKR)
            self.screen.write('--')
        else:
            try:
                r = round(abs(20 * log ((b/x)) / log(10.0)))
            except OverflowError:
                r = 0.0

            self.screen.setInsert(PEAKR)
            if r >= 10.0:
                self.screen.write(`r`[:2])
            elif r >= 1.0:
                self.screen.write(' ' + `r`[:1])            
            else:
                self.screen.write(`r`[1:2])

        
    def startRecording(self, sender):
	sender.rec_started = self.rec_started = time.time()
        self.cur_ws = config.ws
        self.cur_sr = config.sr
	filename = os.path.join(STORAGE, "%4d-%02d-%02d_%02d-%02d-%02d.flac" % time.localtime()[:6])
	self.t.startRecording(filename)

    def pulse(self, sender):
        if not self.t.gotSignal():
            sender.screen.setInsert(STATUS)           
            sender.screen.write("NO SIGNAL ")
            if self.flash:
                sender.screen.write("*")
                self.flash = 0
            else:
                sender.screen.write(" ")
                self.flash = 1          
        else:

            n = tools.format_seconds(time.time() - sender.rec_started)
            if n != self._elapsed:
                sender.screen.setInsert(ELAPSED)
                sender.screen.write(n)
                self._elapsed = n
            self.drawPeaks()
            
class MainMenu(MenuState):
    def __init__(self, last_state):
	MenuState.__init__(self, last_state)
	self.items = [RecSetupMenu(self),
		      SystemMenu(self),
		       DummyMenu("File Tools")]
	self.itemStr = "Main Menu"

class RecSetupMenu(MenuState):
    def __init__(self, last_state):
	MenuState.__init__(self, last_state)
	self.items = [SampleRateMenu(self),
                      WordLengthMenu(self),
		      DummyMenu("Timer"),
                      DummyMenu("Format")]
        self.itemStr = "Recording Setup"

class SystemMenu(MenuState):
    def __init__(self, last_state):
	MenuState.__init__(self, last_state)
        l = [ShutDown(self),
             Reboot(self)]
        l.append(Bounce(self))
        self.items = l
        self.itemStr = "System"

class ShutDown(MenuState):
    def __init__(self, last_state):
	MenuState.__init__(self, last_state)
	self.items = [DummyMenu("Are you sure you", selectable=0),
                      DummyMenu("want to SHUT DOWN?", selectable=0),                      
                      DummyMenu("No"),
		      DummyMenu("Yes"),
                      ]
    def item_str(self):
        return "Shut Down"

    def handle_enter(self, sender):
        a = self.items[self.selected].item_str()
        if a == 'Yes':
            sender.screen.clear()
            sender.screen.write('Shutting down...')
            sender.screen.setInsert((1,2))
            os.system("shutdown -h now")

        else:
            sender.state = self.last_state
            sender.state.entered_state(sender)

class Reboot(MenuState):
    def __init__(self, last_state):
	MenuState.__init__(self, last_state)
	self.items = [DummyMenu("Are you sure you", selectable=0),
                      DummyMenu("want to REBOOT?", selectable=0),                      
                      DummyMenu("No"),
		      DummyMenu("Yes"),
                      ]
        
    def item_str(self):
        return "Reboot"

    def handle_enter(self, sender):
        a = self.items[self.selected].item_str()
        if a == 'Yes':
            sender.screen.clear()
            sender.screen.write("Rebooting...")
            sender.screen.setInsert((1,2))            
            os.system("reboot")
        else:
            sender.state = self.last_state
            sender.state.entered_state(sender)

class Bounce(MenuState):
    def __init__(self, last_state):
	MenuState.__init__(self, last_state)
	self.items = [DummyMenu("Are you sure you", selectable=0),
                      DummyMenu("want to BOUNCE?", selectable=0),                      
                      DummyMenu("No"),
		      DummyMenu("Yes"),
                      ]
    def item_str(self):
        return "Bounce Core"

    def handle_enter(self, sender):
        a = self.items[self.selected].item_str()
        if a == 'Yes':
            sender.screen.clear()
            sender.screen.write("bouncing core...")
            sender.screen.setInsert((1,2))            
            sys.exit(0)
                
        sender.state = self.last_state
        sender.state.entered_state(sender)

    def is_enterable(self):
        if not isinstance(self.last_state.last_state.last_state, RecordingState):
            return 1
        return 0


class SampleRateMenu(MenuState):
    def __init__(self, last_state):
	MenuState.__init__(self, last_state)
	self.items = [DummyMenu("44100"),
		      DummyMenu("48000"),
                      DummyMenu("88200"),
                      DummyMenu("96000")]
    def item_str(self):
        return "Sample Rate: %s" % config.sr
    
    def handle_enter(self, sender):
        config.sr = int(self.items[self.selected].item_str())
        config.save()
        sender.state = self.last_state
        sender.state.entered_state(sender)

class WordLengthMenu(MenuState):
    def __init__(self, last_state):
	MenuState.__init__(self, last_state)
	self.items = [DummyMenu("16"),
		      DummyMenu("24")]

    def item_str(self):
        return "Word Length: %s" % config.ws
    
    def handle_enter(self, sender):
        config.ws = int(self.items[self.selected].item_str())
        config.save()
        sender.state = self.last_state
        sender.state.entered_state(sender)
