import const
import os
import ConfigParser

config = ConfigParser.ConfigParser()

def save():
    configfile = open(const.CONFIG_FILE, 'wb')
    config.write(configfile)

if not os.path.exists(const.CONFIG_FILE):
    config.add_section("Recording")
    config.set("Recording", "sr", '96000')
    config.set("Recording", "ws", '24')
    config.set("Recording", "type", const.STEREO)
    config.set("Recording", "channels", '2')
    save()

else:
    config.read(const.CONFIG_FILE)
