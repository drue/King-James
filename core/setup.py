#!/usr/bin/env python

from distutils.core import setup, Extension

setup(name='transport',
      version='1.0',
      description='James Transport Module',
      author='Andrew Loewenstern',
      author_email='drue@gigagig.org',
      ext_modules=[Extension('transport', ['Transport.cc', 'memq.cc', 'port.cc', 'tportmodule.cc'],
                               libraries=['FLAC','asound'],
                               define_macros=[('DEBUG', None), ('_LARGEFILE64_SOURCE', None)]
     )])

