import const
import bencode

sr = 96000
ws = 24


def save():
    d = {}
    d['sr'] = sr
    d['ws'] = ws
    
    f = open(const.CONFIG_FILE, 'w')
    f.write(bencode.bencode(d))
    f.close()

try:
    f = open(const.CONFIG_FILE, 'r')
except IOError:
    pass
else:
    d = bencode.bdecode(f.read())
    f.close()
    
    sr = d['sr']
    ws = d['ws']

