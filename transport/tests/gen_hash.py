#!/usr/bin/env python

from struct import pack
from md5 import md5

def genblocks(n, samples_per_block=32, bps=24):
    s = ""
    for i in range(n*samples_per_block):
        if (bps == 24):
            s += pack("<i", i)[:-1] # byte aligned samples
        else:
            s += pack("<h", i)
    return hashToCLiteral(md5(s).hexdigest())

def hashToCLiteral(hash):
    return "\\x"+hash[0:2] + hashToCLiteral(hash[2:]) if hash else ""
    
