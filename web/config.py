from ConfigParser import ConfigParser
import sys

if sys.DEV_MODE:
    conf_path = "james.conf"
else:
    conf_path = "/etc/james/james.conf"

config = ConfigParser()
config.read(conf_path)

def save_config():
    config.write(open(conf_path, "w"))
