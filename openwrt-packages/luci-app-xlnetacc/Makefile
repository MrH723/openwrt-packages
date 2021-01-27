include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-xlnetacc
PKG_VERSION:=1.0.9-20201124
PKG_RELEASE:=1

PKG_LICENSE:=GPLv2
PKG_MAINTAINER:=Sense <sensec@gmail.com>

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=luci
	CATEGORY:=LuCI
	SUBMENU:=3. Applications
	TITLE:=LuCI Support for XLNetAcc
	PKGARCH:=all
	DEPENDS:=+jshn +curl +openssl-util +luci-compat
endef

define Package/$(PKG_NAME)/description
	LuCI Support for XLNetAcc.
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/postinst
#!/bin/sh
if [ -z "$${IPKG_INSTROOT}" ]; then
	( . /etc/uci-defaults/luci-xlnetacc ) && rm -f /etc/uci-defaults/luci-xlnetacc
fi
exit 0
endef

define Package/$(PKG_NAME)/conffiles
/etc/config/xlnetacc
endef

define Package/luci-app-xlnetacc/install
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci
	cp -pR ./luasrc/* $(1)/usr/lib/lua/luci
	$(INSTALL_DIR) $(1)/
	cp -pR ./root/* $(1)/
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/i18n
	po2lmo ./po/zh_Hans/xlnetacc.po $(1)/usr/lib/lua/luci/i18n/xlnetacc.zh-cn.lmo
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
