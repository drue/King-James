from zmq.eventloop import ioloop

il = ioloop.IOLoop.instance()

def peak_producer(context): 
    """ some canned peak data for testing the UI """
    socket = context.socket(zmq.PUB) 
    socket.bind('inproc://peaks')
    

    def peaks(file):
        f = open(file, "r")
        while True:
            for line in f:
                yield line
            f.seek(0)
    p = peaks("peakz")

    def doIt():
        socket.send(p.next()) 
        il.add_timeout(time() + 0.10, doIt)
 
    doIt()
