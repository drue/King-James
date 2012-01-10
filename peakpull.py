#!/usr/bin/env python

import sys
import wave
from struct import Struct
import math

if __name__ == '__main__':
    output = open("peakz", 'w')
    print ">>>", sys.argv[1]
    f = wave.open(sys.argv[1])
    window = int(441000 * 0.1)
    unpack = Struct("<hh").unpack_from
    frame = f.readframes(window)
    while frame:
        size = len(frame)
        i = 0
        maxL = maxR = 0
        while i < size:
            l, r = unpack(frame, i)
            i += 4
            if l > maxL:
                maxL = l
            if r > maxR:
                maxR = r
        maxL = 20 * math.log10(maxL / 32767.0)
        maxR = 20 * math.log10(maxR / 32767.0)
        output.write('[%s, %s]\n' % (maxL, maxR))
        frame = f.readframes(window)
        
    output.close()


    
