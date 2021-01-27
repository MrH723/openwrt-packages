#
# Copyright (C) 2019-2020 Oever González <notengobattery@gmail.com>
#
# Licensed to the public under the Apache License 2.0.
#

include $(TOPDIR)/rules.mk

PKG_MAINTAINER:=Oever Gonzalez <notengobattery@gmail.com>

LUCI_TITLE:=LuCI compressed memory  configuration
LUCI_DESCRIPTION:=\
This package allows users to configure the compressed memory subsystem. This \
GUI allows users to setup basic features, such as the compression algorithm, \
the base memory pool and the hard limit. Also, allows them to choose between \
using either zswap, zram or both without to enable or disable the boot \
scripts. Advanced setup and fine tunning can be archived by using the UCI \
interface.

LUCI_DEPENDS:=+luci-base +swap-zram-writeback +swap-zswap-cache
LUCI_PKGARCH:=all
PKG_RELEASE:=4

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature
