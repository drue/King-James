class DummyTransport:
    pos = 0
    vals = [0.0, 2**8.0, 2**12, 2**16.0, 2**20.0, 2**21, 2**22, 2**23.0]
    def startRecording(*args, **kwargs):
        pass
    def stop(*args, **kwargs):
        pass
    def waitTillFinished(*args, **kwargs):
        pass
    def getPeaks(*args, **kwargs):
        self = args[0]
        t = self.vals[self.pos]
        self.pos = (self.pos + 1) % len(self.vals)
        return (t,t)
    def resetPeaks(*args, **kwargs):
        pass
    def gotSignal(*args, **kwargs):
        return 1

def newTPort(*args, **kwargs):
    return DummyTransport()
