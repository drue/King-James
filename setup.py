#!/usr/bin/env python

from distutils.core import setup, Extension

setup(name='james',
      version='1.0',
      description='King James Recorder',
      author='Andrew Loewenstern',
      author_email='drue@gigagig.org',
      packages=['web'],
      scripts=['bin/james'],
      package_data={"web":["index.html", "static/*.png", "static/*.js", "static/*.css"]},
      ext_modules=[Extension('transport', ['transport/Transport.cc', 'transport/spool.cc', 'transport/meter.cc', 'transport/tportmodule.cc'],
                             libraries=['FLAC','asound', 'zmq', 'jack', 'boost_thread'],
                             define_macros=[('DEBUG', None), ('_LARGEFILE64_SOURCE', None)],
                             # extra_compile_args=["-O0"]
          )])

