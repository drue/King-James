import time
import os
import string
import tools
import config

try:
    import transport
except ImportError:
    import dummytransport as transport


from const import *

# state classes for UI

class BaseState:
    _lt = ''
    def __init__(self):
	pass
    def key_down(self, key, sender):
	try:
	    f = getattr(self, "handle_" + KEYMAP[key])
	except KeyError:
	    pass
	except AttributeError:
	    pass
	else:
	    f(sender)
	    
    def pulse(self, sender):
	pass
    
    def handle_enter(self, sender):
	if sender.light:
	    sender.screen.lightOff()
	    sender.light = 0
	else:
	    sender.screen.lightOn()
	    sender.light = 1
	    
    def entered_state(self, sender):
	self.screen = sender.screen

    def redraw_screen(self):
	pass

    def showTime(self):
	t = time.localtime()
        x = '%02d:%02d   REM %s' % (t[3], t[4], tools.timeleft(sr=config.sr, ws=config.ws))
        if x != self._lt:
            self._lt = x
            self.screen.setInsert(TIME)
            self.screen.write(self._lt)

class MenuState(BaseState):
    def __init__(self, last_state, selectable = 1):
	BaseState.__init__(self)
	self.last_state = last_state
	self.items = []
	self.selected = None
        self.selectable = selectable
        self.top_item = 0
	self.itemStr = "Menu Choice"
        
    def entered_state(self, sender):
	BaseState.entered_state(self, sender)
	assert len(self.items) > 0
        for i in self.items:
            if i.is_selectable():
                self.selected = self.items.index(i)
                break
        assert self.selected != None
	self.redraw_screen()

    def handle_left(self, sender):
	t = self.selected	
	while t > 0:
	    t -= 1
	    if self.items[t].is_selectable():
                self.change_selected_row(t)
		break
	    
    def handle_right(self, sender):
	t = self.selected
	while t < (len(self.items) - 1):
	    t += 1
	    if self.items[t].is_selectable():
                self.change_selected_row(t)
		break

    def change_selected_row(self, new):
        orow = self.selected_row()
        self.selected = new
        nrow = self.selected_row()
        #print ">>>", nrow
        if nrow <= 1 and self.selected > 0:
            # scroll up
            if self.selected <= (self.screen.DIM_Y - 1):
                self.top_item = 0
            else:
                self.top_item = self.selected - self.screen.DIM_Y + 1
            self.redraw_screen()
            #print "A>>> top", self.top_item, "sel", self.selected
        elif nrow >= self.screen.DIM_Y:
            # scroll down
            if len(self.items) - self.selected <= self.screen.DIM_Y:
                self.top_item = len(self.items) - self.screen.DIM_Y
                self.change_selected_icon(orow, nrow)
            else:
                self.top_item = self.selected
                self.redraw_screen()
            #print "B>>> top", self.top_item, "sel", self.selected

        else:
            nrow = self.selected_row()
            self.change_selected_icon(orow, nrow)
        
    def selected_row(self):
        # rows start at 1!!
        return (self.selected - self.top_item) + 1
    
    def handle_menu(self, sender):
	sender.state = self.last_state
	sender.state.entered_state(sender)

    def handle_enter(self, sender):
	state = self.items[self.selected]
	if state.is_enterable():
	    sender.state = state
	    sender.state.entered_state(sender)

    def change_selected_icon(self, old, new, icon = '*'):
        assert len(icon) == 1

        self.screen.setInsert((1, old))
        self.screen.write(' ')
        self.screen.setInsert((self.screen.DIM_X, old))
        self.screen.write(' ')

        self.screen.setInsert((1, new))
        self.screen.write(icon)
        self.screen.setInsert((self.screen.DIM_X, new))
        self.screen.write(icon)

        if old == 1 and self.top_item != 0:
            self.screen.setInsert((1,1))
            self.screen.write('<')
        if old == self.screen.DIM_Y and (self.top_item + self.screen.DIM_Y) <= len(self.items) - 1:
            self.screen.setInsert((self.screen.DIM_X, self.screen.DIM_Y))
            self.screen.write('>')
    def redraw_screen(self):
	self.screen.clear()
        for i in range(len(self.items)):
            if i < self.top_item or i - self.top_item + 1 > self.screen.DIM_Y:
                continue
	    str = self.items[i].item_str()[:self.screen.DIM_X - 2]
	    str = str + ' ' * ((self.screen.DIM_X - 2) - len(str))
	    if i == self.selected:
		self.screen.setInsert((1, (i - self.top_item)+1))
		self.screen.write('*')
		self.screen.write(str)
		self.screen.write('*')
            elif i == self.top_item and i > 0:
		self.screen.setInsert((1, (i - self.top_item)+1))
		self.screen.write('<')
		self.screen.write(str)
            elif i == self.top_item + self.screen.DIM_Y and i < len(self.items) - 1:
		self.screen.setInsert((2, (i - self.top_item)+1))
		self.screen.write(str)
		self.screen.write('>')
	    else:
		self.screen.setInsert((2, (i - self.top_item)+1))
		self.screen.write(str)
		
    def item_str(self):
	# string shown in parent menus list
	return self.itemStr
    
    def is_enterable(self):
	# is enterable
	return 1

    def is_selectable(self):
	# is selectable
	return self.selectable

class DummyMenu(MenuState):
    def __init__(self, name, selectable=1):
	self.itemStr = name
        self.selectable=selectable
    def is_enterable(self):
	return 0

class OptionState(BaseState):
    def __init__(self, last_state):
	BaseState.__init__(self)
	self.last_state = last_state
	self.options = {}
	self.selected = 0

    def entered_state(self, sender):
	BaseState.entered_state(self, sender)
	assert len(self.options) > 0
        self.row = self.last_state.selected_row()
        


    
