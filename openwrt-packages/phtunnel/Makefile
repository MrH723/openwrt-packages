#
# Copyright (C) 2007-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=phtunnel
PKG_VERSION:=1.0.0
PKG_RELEASE:=3
FILES_PATH:=files

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Web Servers/Proxies
  TITLE:=phtunnel
  DEPENDS:=+librt +libpthread +libc +wget @!PACKAGE_phddns
endef


ifeq ($(ARCH),x86_64)
	PKG_ARCH:=amd64
endif
ifeq ($(ARCH),mipsel)
	PKG_ARCH:=mipsel
endif
ifeq ($(ARCH),mips)
	PKG_ARCH:=mips
endif
ifeq ($(ARCH),i386)
	PKG_ARCH:=x86
endif
ifeq ($(ARCH),arm)
	PKG_ARCH:=arm
endif
ifeq ($(ARCH),aarch64)
	PKG_ARCH:=arm64
endif

define Package/$(PKG_NAME)/description
PHTunnel
endef

define Package/$(PKG_NAME)/conffiles
/etc/phtunnel.json
/etc/config/phtunnel
endef

define Package/$(PKG_NAME)/extra_provides
    echo 'libc.so.6'; \
    echo 'libm.so.6'; \
    echo 'libc.so.0'; \
    echo 'libm.so.0'; \
    echo 'libpthread.so.0';
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
	true
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(INSTALL_DIR) $(1)/lib/upgrade/keep.d

	$(INSTALL_BIN) $(FILES_PATH)/usr/bin/$(PKG_ARCH)/phtunnel $(1)/usr/bin
	$(INSTALL_BIN) $(FILES_PATH)/etc/init.d/phtunnel $(1)/etc/init.d
	$(INSTALL_BIN) $(FILES_PATH)/etc/config/phtunnel $(1)/etc/config
	$(INSTALL_BIN) $(FILES_PATH)/etc/hotplug.d/iface/30-oray-phtunnel $(1)/etc/hotplug.d/iface
	$(INSTALL_BIN) $(FILES_PATH)/lib/upgrade/keep.d/phtunnel $(1)/lib/upgrade/keep.d
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
