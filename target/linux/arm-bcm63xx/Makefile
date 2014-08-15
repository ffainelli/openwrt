#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=arm-bcm63xx
BOARDNAME:=Broadcom BCM63xx with ARM CPU
FEATURES:=squashfs usb pci pcie gpio
MAINTAINER:=Florian Fainelli <florian@openwrt.org>
CPU_TYPE:=cortex-a9

KERNEL_PATCHVER:=3.19

include $(INCLUDE_DIR)/target.mk

define Target/Description
	Build firmware images for Broadcom based BCM63xx routers with ARM CPU, *not* MIPS.
endef

KERNELNAME:=zImage dtbs

DEFAULT_PACKAGES += swconfig wpad-mini

$(eval $(call BuildTarget))
