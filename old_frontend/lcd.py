
# command byte
CMD = '\xfe'

## text commands
LWRAP_ON = '\x43'
LWRAP_OFF = '\x44'
ASCROLL_ON = '\x51'
ASCROLL_OFF = '\x52'
SET_INSERT = '\x47'
INSERT_HOME = '\x48'
INSERT_AT_PIXEL = '\x79'
SET_FONT = '\x31'
SET_FONT_METRICS = '\x32'

## graphs
INIT_GRAPH = '\x67'
SET_GRAPH = '\x69'

## misc
CLEAR = '\x58'
SET_CONTRAST = 	'\x50'
LIGHT_ON = '\x42'
LIGHT_OFF = '\x46'
GPO_ON = '\x56'
GPO_OFF = '\x57'


DIM_X = 20
DIM_Y = 4



class LCDWriter:
    def __init__(self, fd):
        global DIM_X
        global DIM_Y
	self.fd = fd
	self.graphs = 0
	self.DIM_X = DIM_X
        self.DIM_Y = DIM_Y
        
    # send a string down the pipe
    def write(self, txt):
	self.fd.write(txt)
	self.fd.flush()

    # formats a command and numeric arguments before writing
    def _sendCommand(self, command, *args):
	str = CMD + command
	for arg in args:
	    str = str + chr(arg)
	self.write(str)
	
    # timeout = minutes until light goes out, default = stay on
    def lightOn(self, timeout=0):
	self._sendCommand(LIGHT_ON, timeout)
	
    def lightOff(self):
	self._sendCommand(LIGHT_OFF)
	
    def clear(self):
	self._sendCommand(CLEAR)
    
    # sets text insert point at character col/row
    def setInsert(self, coords):
	self._sendCommand(SET_INSERT, coords[0], coords[1])

    # sets textinsert point at pixel x/y
    def setInsertAtPixel(self, coords):
    	self._sendCommand(INSERT_AT_PIXEL, coords[0], coords[1])

    # warning, there are only 15 slots for graphs
    # if you create more than 15 graphs then they are recycled in FIFO order
    def newGraphVertFromBottom(self, x1, y1, x2, y2):
	self.graphs = 1 + self.graphs % 15  # watch out!
	self._sendCommand(INIT_GRAPH, self.graphs, 0, x1, y1, x2, y2)
	return Graph(self, self.graphs, y2-y1)
	
    def newGraphHorisFromLeft(self, x1, y1, x2, y2):
	self.graphs = 1 + self.graphs % 15  # watch out!
	self._sendCommand(INIT_GRAPH, self.graphs, 1, x1, y1, x2, y2)
	return Graph(self, self.graphs, x2-x1)

    def newGraphVertFromTop(self, x1, y1, x2, y2):
	self.graphs = 1 + self.graphs % 15  # watch out!
	self._sendCommand(INIT_GRAPH, self.graphs, 2, x1, y1, x2, y2)
	return Graph(self, self.graphs, y2-y1)

    def newGraphHorizFromRight(self, x1, y1, x2, y2):
	self.graphs = 1 + self.graphs % 15  # watch out!
	self._sendCommand(INIT_GRAPH, self.graphs, 3, x1, y1, x2, y2)
	return Graph(self, self.graphs, x2-x1)

	
# encapsulate a graph
class Graph:
    def __init__(self, lcd, id, size):
	self.id = id
	self.size = size
	self.lcd = lcd
    
    def updateAbsolute(self, val):
	self.lcd._sendCommand(SET_GRAPH, self.id, val)
    
    def updatePercentage(self, p):
	val = round(self.size * (p / 100.0))
	self.updateAbsolute(val)
    


class DummyWriter:
    def __init__(self, fd):
        global DIM_X
        global DIM_Y
        self.fd = fd
        self.graphs = 0
        self.light = 1
        self.cur_insert = (0, 0)
        self.clear()
	self.DIM_X = DIM_X
        self.DIM_Y = DIM_Y
        
    def write(self, txt):
        txt = txt[:self.DIM_X - self.cur_insert[0]]
        row = self.buf[self.cur_insert[1]]
        tmp = row[:self.cur_insert[0]] + txt + row[self.cur_insert[0] + len(txt):]
        assert len(tmp) == self.DIM_X
        self.buf[self.cur_insert[1]] = tmp
        self.cur_insert = (self.cur_insert[0] + len(txt), self.cur_insert[1])
        self.flush()
        
    def lightOn(self):
        self.light = 1

    def lightOff(self):
        self.light = 0

    def clear(self):
        self.buf = []
        for x in range(DIM_Y):
            self.buf.append(' '*20)

    def setInsert(self, coords):
        coords = (coords[0] - 1, coords[1] - 1)
        assert coords[0] <= DIM_X
        assert coords[0] >= 0        
        assert coords[1] <= DIM_Y
        assert coords[1] >= 0
        self.cur_insert = coords

    def flush(self):
        print " " + "-" * DIM_X
        for buf in self.buf:
            print '|' + buf + '|'
        print " " + "-" * DIM_X
        
