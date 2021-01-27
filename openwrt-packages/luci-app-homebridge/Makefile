include $(TOPDIR)/rules.mk

LUCI_TITLE:=LuCI Support for homebridge
LUCI_DEPENDS:=+node

PKG_VERSION:=0.2.0
PKG_RELEASE:=1
PKG_DATE:=20201026
PKG_MAINTAINER:=lanxin Shang <shanglanxin@gmail.com>
PKG_LICENSE:=GPL-3.0-or-later

define Package/luci-app-homebridge/postinst
#!/bin/sh
mkdir $${IPKG_INSTROOT}/etc/homebridge/ >/dev/null 2>&1
exit 0
endef

define Package/luci-app-homebridge/conffiles
/etc/config/homebridge
endef

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature
