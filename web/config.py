from ConfigParser import ConfigParser

config = ConfigParser()
config.read("/etc/james/james.conf")

def save_config():
    config.write(open("/etc/qames/james.conf", "w"))
