# define LINUX_EXTRACT_CMDS
# 	bzcat $(DL_DIR)/$(LINUX_SOURCE) | tar -C $(@D) -xf -
# endef

include $(sort $(wildcard $(BR2_EXTERNAL)/package/*/*.mk))
