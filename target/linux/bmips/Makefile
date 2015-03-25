#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=mipsel
BOARD:=bmips
BOARDNAME:=Broadcom BMIPS generic kernel
FEATURES:=squashfs usb atm pci pcmcia usbgadget
KERNEL_PATCHVER:=4.0
MAINTAINER:=Florian Fainelli <florian@openwrt.org>

include $(INCLUDE_DIR)/target.mk


define Target/Description
	Build firmware images for Broadcom BMIPS-based SoCs
endef

$(eval $(call BuildTarget))
