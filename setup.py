#!/usr/bin/env python

from distutils.core import setup, Extension
from multiprocessing import cpu_count

# monkey-patch for parallel compilation
def parallelCCompile(self, sources, output_dir=None, macros=None, include_dirs=None, debug=0, extra_preargs=None, extra_postargs=None, depends=None):
    # those lines are copied from distutils.ccompiler.CCompiler directly
    macros, objects, extra_postargs, pp_opts, build = self._setup_compile(output_dir, macros, include_dirs, sources, depends, extra_postargs)
    cc_args = self._get_cc_args(pp_opts, debug, extra_preargs)
    # parallel code
    N=cpu_count()
    import multiprocessing.pool
    def _single_compile(obj):
        try: src, ext = build[obj]
        except KeyError: return
        self._compile(obj, src, ext, cc_args, extra_postargs, pp_opts)
    # convert to list, imap is evaluated on-demand
    list(multiprocessing.pool.ThreadPool(N).imap(_single_compile,objects))
    return objects
import distutils.ccompiler
distutils.ccompiler.CCompiler.compile=parallelCCompile


setup(name='james',
      version='1.0',
      description='King James Recorder',
      author='Andrew Loewenstern',
      author_email='drue@gigagig.org',
      packages=['web'],
      scripts=['bin/james'],
      package_data={"web":["index.html", "static/*.png", "static/*.js", "static/*.css", "static/*.map", "static/css/*.css"]},
      ext_modules=[Extension('transport', ['transport/Transport.cc', 'transport/spool.cc', 'transport/meter.cc', 'transport/tportmodule.cc'],
                             depends=['transport/Transport.h', 'transport/spool.h', 'transport/meter.h', 'transport/const.h'],
                             libraries=['FLAC','asound', 'zmq', 'jack', 'boost_thread', 'boost_system'],
                             define_macros=[('DEBUG', None), ('_LARGEFILE64_SOURCE', None)],
                             # extra_compile_args=["-O0"]
          )])

