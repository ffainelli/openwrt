#
# Copyright (C) 2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=ldconfig-native
PKG_VERSION:=2.12.1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://cgit.openembedded.org/openembedded-core/plain/meta/recipes-core/glibc/$(PKG_NAME)-$(PKG_VERSION)/
PKG_MD5SUM:=bc75cfa6ea9847b7c20e00e7d10ff546

HOST_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/host-build.mk

define Host/Compile
	(cd $(HOST_BUILD_DIR)/; \
	$(HOSTCC) $(HOST_CFLAGS) ldconfig.c -std=gnu99 chroot_canon.c \
		xmalloc.c xstrdup.c cache.c readlib.c  -I. dl-cache.c -o ldconfig; )
endef

define Host/Install
	$(CP) $(HOST_BUILD_DIR)/ldconfig $(STAGING_DIR_HOST)/bin/ldconfig
endef

$(eval $(call HostBuild))
