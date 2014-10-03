################################################################################
#
# hostapd - with rtl871xdrv
#
################################################################################

RTL_HOSTAPD_VERSION = 2.2
RTL_HOSTAPD_SITE = http://hostap.epitest.fi/releases
RTL_HOSTAPD_SOURCE = hostapd-$(RTL_HOSTAPD_VERSION).tar.gz
RTL_HOSTAPD_SUBDIR = hostapd
RTL_HOSTAPD_CONFIG = $(RTL_HOSTAPD_DIR)/$(RTL_HOSTAPD_SUBDIR)/.config
RTL_HOSTAPD_DEPENDENCIES = libnl
RTL_HOSTAPD_CFLAGS = $(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include/libnl3/
RTL_HOSTAPD_LICENSE = GPLv2/BSD-3c
RTL_HOSTAPD_LICENSE_FILES = README
RTL_HOSTAPD_CONFIG_SET =

RTL_HOSTAPD_CONFIG_ENABLE = \
	CONFIG_ACS \
	CONFIG_FULL_DYNAMIC_VLAN \
	CONFIG_HS20 \
	CONFIG_IEEE80211AC \
	CONFIG_IEEE80211N \
	CONFIG_IEEE80211R \
	CONFIG_INTERNAL_LIBTOMMATH \
	CONFIG_INTERWORKING \
	CONFIG_LIBNL32 \
	CONFIG_VLAN_NETLINK

RTL_HOSTAPD_CONFIG_DISABLE =

# libnl-3 needs -lm (for rint) and -lpthread if linking statically
# And library order matters hence stick -lnl-3 first since it's appended
# in the hostapd Makefiles as in LIBS+=-lnl-3 ... thus failing
ifeq ($(BR2_PREFER_STATIC_LIB),y)
RTL_HOSTAPD_LIBS += -lnl-3 -lm -lpthread
endif

ifeq ($(BR2_INET_IPV6),)
	RTL_HOSTAPD_CONFIG_DISABLE += CONFIG_IPV6
endif

# Try to use openssl if it's already available
ifeq ($(BR2_PACKAGE_OPENSSL),y)
	RTL_HOSTAPD_DEPENDENCIES += openssl
	RTL_HOSTAPD_LIBS += $(if $(BR2_PREFER_STATIC_LIB),-lcrypto -lz)
	RTL_HOSTAPD_CONFIG_EDITS += 's/\#\(CONFIG_TLS=openssl\)/\1/'
else
	RTL_HOSTAPD_CONFIG_DISABLE += CONFIG_EAP_PWD
	RTL_HOSTAPD_CONFIG_EDITS += 's/\#\(CONFIG_TLS=\).*/\1internal/'
endif

ifeq ($(BR2_PACKAGE_RTL_HOSTAPD_EAP),y)
	RTL_HOSTAPD_CONFIG_ENABLE += \
		CONFIG_EAP \
		CONFIG_RADIUS_SERVER \

# Enable both TLS v1.1 (CONFIG_TLSV11) and v1.2 (CONFIG_TLSV12)
	RTL_HOSTAPD_CONFIG_ENABLE += CONFIG_TLSV1
else
	RTL_HOSTAPD_CONFIG_DISABLE += CONFIG_EAP
	RTL_HOSTAPD_CONFIG_ENABLE += \
		CONFIG_NO_ACCOUNTING \
		CONFIG_NO_RADIUS
endif

ifeq ($(BR2_PACKAGE_RTL_HOSTAPD_WPS),y)
	RTL_HOSTAPD_CONFIG_ENABLE += CONFIG_WPS
endif

define RTL_HOSTAPD_CONFIGURE_CMDS
	cp $(BR2_EXTERNAL)/package/rtl-hostapd/config $(RTL_HOSTAPD_CONFIG)
endef

define RTL_HOSTAPD_BUILD_CMDS
	$(TARGET_MAKE_ENV) CFLAGS="$(RTL_HOSTAPD_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" LIBS="$(RTL_HOSTAPD_LIBS)" \
		$(MAKE) CC="$(TARGET_CC)" -C $(@D)/$(RTL_HOSTAPD_SUBDIR)
endef

define RTL_HOSTAPD_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 -D $(@D)/$(RTL_HOSTAPD_SUBDIR)/hostapd \
		$(TARGET_DIR)/usr/sbin/hostapd
	$(INSTALL) -m 0755 -D $(@D)/$(RTL_HOSTAPD_SUBDIR)/hostapd_cli \
		$(TARGET_DIR)/usr/bin/hostapd_cli
endef

$(eval $(generic-package))
