# Copyright (C) 2016 Openwrt.org
#
# This is free software, licensed under the Apache License, Version 2.0 .
#

include $(TOPDIR)/rules.mk

LUCI_TITLE:=LuCI support for Amule
LUCI_DEPENDS:=+amule +antileech
LUCI_PKGARCH:=all

define Package/luci-app-amule/conffiles
	/etc/config/amule
endef

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature

