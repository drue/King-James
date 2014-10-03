################################################################################
#
# python-james
#
################################################################################

PYTHON_JAMES_VERSION = 1.0
PYTHON_JAMES_SOURCE = james-$(PYTHON_JAMES_VERSION).tar.gz
PYTHON_JAMES_SITE = file:////dist
PYTHON_JAMES_LICENSE = Proprietary
PYTHON_JAMES_SETUP_TYPE = distutils

$(eval $(python-package))
