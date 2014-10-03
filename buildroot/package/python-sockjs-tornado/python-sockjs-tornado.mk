################################################################################
#
# python-sockjs-tornado
#
################################################################################

PYTHON_SOCKJS_TORNADO_VERSION = b68096e9d30d1e37e39f6d502dbaf46f688704c3
PYTHON_SOCKJS_TORNADO_SITE = https://github.com/mrjoes/sockjs-tornado.git
PYTHON_SOCKJS_TORNADO_SITE_METHOD = git
PYTHON_SOCKJS_TORNADO_LICENSE = MIT
PYTHON_SOCKJS_TORNADO_SETUP_TYPE = distutils

$(eval $(python-package))
