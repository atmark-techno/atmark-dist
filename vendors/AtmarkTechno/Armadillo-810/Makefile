#
# Makefile -- Build instructions
#

include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
ifeq ($(CONFIG_USER_BUSYBOX_1_20_2),y)
-include $(ROOTDIR)/user/busybox/busybox-1.20.2/.config
endif
include $(ARCH_CONFIG)
include $(ROOTDIR)/version
include ../Common/Armadillo.mk

DEPMOD_PL = ../Common/tools/depmod.pl
PREBUILD_LIBDIR = $(ROOTDIR)/lib/prebuild/$(CROSS_COMPILE:-=)
PREBUILD_PRODUCTDIR = prebuild/$(CROSS_COMPILE:-=)
LINUX_VERSION = $(shell make --no-print-directory -C $(ROOTDIR)/$(LINUXDIR) \
			     kernelversion)
ROOT_PASSWD := \
	$(shell perl $(ROOTDIR)/tools/crypt.pl $(CONFIG_VENDOR_ROOT_PASSWD))

comma := ,
empty :=
space := $(empty) $(empty)

SUBDIR_y =
SUBDIR_$(CONFIG_VENDOR_CONVERT_PUBKEY_CONVERT_PUBKEY)  += convert_pubkey/
SUBDIR_$(CONFIG_VENDOR_AWL12_AERIAL)		+= awl12/
SUBDIR_$(CONFIG_VENDOR_AWL13_AWL13)		+= awl13/

all:
	for i in $(SUBDIR_y) ; do $(MAKE) -C $$i || exit $? ; done

clean:
	-for i in $(SUBDIR_y) ; do [ ! -d $$i ] || $(MAKE) -C $$i clean; done

distclean: clean
	rm -f config.$(LINUXDIR) etc/DISTNAME

#
# init scripts
#   rc.d/S0*script: System core
#   rc.d/S1*script: System core
#   rc.d/S2*script: File permission
#   rc.d/S3*script: Log daemon
#   rc.d/S4*script: File System, Module
#   rc.d/S5*script: Basic, Network
#   rc.d/S6*script: Network daemon
#   rc.d/S7*script: misc
#   rc.d/S8*script: misc
#   rc.d/S9*script: Configurable
#
_INETD_y =
_INETD_$(CONFIG_USER_INETD_INETD)		+= y
_INETD_$(CONFIG_USER_BUSYBOX_1_00_RC3)		+= $(CONFIG_USER_BUSYBOX_INETD)
_INETD_$(CONFIG_USER_BUSYBOX_1_20_2)		+= $(CONFIG_INETD)
INETD_y = $(findstring y, $(_INETD_y))

INITSCRIPTS_y =
INITSCRIPTS_y					+= /etc/init.d/mtd,01
INITSCRIPTS_$(CONFIG_USER_FLATFSD_FLATFSD)	+= /etc/init.d/flatfsd,03
INITSCRIPTS_$(CONFIG_USER_UDEV)			+= /etc/init.d/udevd,05
INITSCRIPTS_y					+= /etc/init.d/mountdevsubfs,06
INITSCRIPTS_y					+= /etc/init.d/checkroot,20
INITSCRIPTS_y					+= /etc/init.d/checkftp,21
INITSCRIPTS_y					+= /etc/init.d/syslogd,30
INITSCRIPTS_y					+= /etc/init.d/klogd,31
INITSCRIPTS_y					+= /etc/init.d/mount,40
INITSCRIPTS_$(CONFIG_USER_UDEV_EXTRA_FIRMWARE) += /etc/init.d/firmware,45
INITSCRIPTS_y					+= /etc/init.d/module-init-tools,45
INITSCRIPTS_y					+= /etc/init.d/hostname,50
INITSCRIPTS_y					+= /etc/init.d/firewall,55
INITSCRIPTS_y					+= /etc/init.d/networking,56
INITSCRIPTS_$(INETD_y)				+= /etc/init.d/inetd,57
INITSCRIPTS_$(CONFIG_USER_AT_CGI_SYSTEM)	+= /etc/init.d/at-cgi,60
INITSCRIPTS_$(CONFIG_USER_AVAHI_AVAHI)		+= /etc/init.d/avahi,60
INITSCRIPTS_$(CONFIG_USER_LIGHTTPD_LIGHTTPD)	+= /etc/init.d/lighttpd,60
INITSCRIPTS_$(CONFIG_USER_UCDSNMP_SNMPD)	+= /etc/init.d/snmpd,60
INITSCRIPTS_$(CONFIG_USER_OPENSSH_SSHD)		+= /etc/init.d/sshd,60
INITSCRIPTS_y					+= /etc/init.d/rc.local,90
INITSCRIPTS = $(shell echo $(INITSCRIPTS_y) | sed -e "s/ /\n/g" | uniq)

generate_initscript_link = \
	@for script in $(INITSCRIPTS); do \
		nr=$${script\#\#*,}; \
		src=$${script%%,*}; \
		dst=$$(printf "/etc/rc.d/S%02d%s" $$nr $$(basename $$src)); \
		printf "  LINK $$src $$dst\n"; \
		$(ROMFSINST) -s $$src $$dst; \
	done

install_getboardinfo = \
	$(ROMFSINST) $(PREBUILD_PRODUCTDIR)/usr/sbin/get-board-info-a810 \
		     /usr/sbin/get-board-info-a810

#
# Armadillo-WLAN
#
AWLAN_ETC_DIR = ../Armadillo-WLAN.Common/etc
AWL13_FIRMWARE_DIR  = ../Armadillo-WLAN.Common/lib/firmware/awl13
get_awl13_firmware  = $(shell ls -1v $(AWL13_FIRMWARE_DIR)/$(1) | tail -1)

AWL13_FWIMAGE_SDIO_STA	= $(call get_awl13_firmware,fwimage*_STA_SDIO.bin)
AWL13_FWIMAGE_SDIO_AP	= $(call get_awl13_firmware,fwimage*_AP_SDIO.bin)
AWL13_FWIMAGE_USB_STA	= $(call get_awl13_firmware,fwimage*_STA_USB.bin)
AWL13_FWIMAGE_USB_AP	= $(call get_awl13_firmware,fwimage*_AP_USB.bin)

AWL13_FWIMAGE_SDIO_$(CONFIG_VENDOR_AWL13_AWL13_STA)= $(AWL13_FWIMAGE_SDIO_STA)
AWL13_FWIMAGE_SDIO_$(CONFIG_VENDOR_AWL13_AWL13_AP) = $(AWL13_FWIMAGE_SDIO_AP)
AWL13_FWIMAGE_USB_$(CONFIG_VENDOR_AWL13_AWL13_STA) = $(AWL13_FWIMAGE_USB_STA)
AWL13_FWIMAGE_USB_$(CONFIG_VENDOR_AWL13_AWL13_AP)  = $(AWL13_FWIMAGE_USB_AP)
AWL13_FWIMAGE_$(CONFIG_VENDOR_AWL13_AWL13_SDIO)    = $(AWL13_FWIMAGE_SDIO_y)
AWL13_FWIMAGE_$(CONFIG_VENDOR_AWL13_AWL13_USB)     = $(AWL13_FWIMAGE_USB_y)
AWL13_MODULE_$(CONFIG_VENDOR_AWL13_AWL13_SDIO)     = awl13_sdio
AWL13_MODULE_$(CONFIG_VENDOR_AWL13_AWL13_USB)      = awl13_usb

install_awl12 = \
	@if [ "$(CONFIG_VENDOR_AWL12_AERIAL)" = "y" ]; then \
		$(ROMFSINST) ../Armadillo-WLAN.Common/lib/firmware/aerial \
			     /lib/firmware/aerial; \
		[ `grep -qx "aerial" $(ROMFSDIR)/etc/modules` ] || \
			echo "aerial" >> $(ROMFSDIR)/etc/modules; \
		$(ROMFSINST) $(AWLAN_ETC_DIR)/default/aerial-firmware-load.sh \
			     /etc/default/aerial-firmware-load.sh; \
		$(ROMFSINST) $(AWLAN_ETC_DIR)/udev/rules.d/z05_aerial.rules \
			     /etc/udev/rules.d/z05_aerial.rules; \
	fi

install_awl13 = \
	@if [ "$(CONFIG_VENDOR_AWL13_AWL13)" = "y" ]; then \
		rm -rf $(ROMFSDIR)/lib/firmware/awl13; \
		mkdir -p $(ROMFSDIR)/lib/firmware/awl13; \
		$(ROMFSINST) $(AWL13_FWIMAGE_y) \
			/lib/firmware/awl13/$(notdir $(AWL13_FWIMAGE_y)); \
		[ `grep -qx "$(AWL13_MODULE_y)" $(ROMFSDIR)/etc/modules` ] || \
			echo "$(AWL13_MODULE_y)" >> $(ROMFSDIR)/etc/modules; \
		$(ROMFSINST) $(AWLAN_ETC_DIR)/default/awl13-firmware-load.sh \
			     /etc/default/awl13-firmware-load.sh; \
		$(ROMFSINST) $(AWLAN_ETC_DIR)/udev/rules.d/z05_awl13.rules \
			     /etc/udev/rules.d/z05_awl13.rules; \
	fi

generate_etc_DISTNAME = \
	@printf "%s v%s (%s/%s)\n" \
	       $(DIST_NAME) $(DIST_VERSION) $(CONFIG_VENDOR) $(CONFIG_PRODUCT) \
	       > $(ROMFSDIR)/etc/DISTNAME

update_root_passward = \
	chmod 644 $(ROMFSDIR)/etc/shadow; \
	sed -i -e "s/^root:.*//" -e "/^$$/d" $(ROMFSDIR)/etc/shadow; \
	$(ROMFSINST) -A "root" -a "root:${ROOT_PASSWD}:1:0:99999:7:::" \
		/etc/shadow

romfs:
	mkdir -p $(ROMFSDIR)
	for i in $(ROMFS_DIRS); do \
		[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)/$$i; \
	done

	for i in $(SUBDIR_y) ; do $(MAKE) -C $$i romfs || exit $? ; done

	$(ROMFSINST) /etc
	$(ROMFSINST) /home
	$(ROMFSINST) /usr

	$(ROMFSINST) ../../Generic/romfs/etc/services /etc/services

	$(ROMFSINST) -s /proc/mounts /etc/mtab

	$(ROMFSINST) -s /etc/config/HOSTNAME /etc/HOSTNAME
	$(ROMFSINST) -s /etc/config/hosts /etc/hosts
	$(ROMFSINST) -s /etc/config/resolv.conf /etc/resolv.conf
	$(ROMFSINST) -s /etc/config/interfaces /etc/network/interfaces

# Fix up permissions
	$(call update_root_passward)
	chmod 400 $(ROMFSDIR)/etc/shadow
	chmod 400 $(ROMFSDIR)/etc/gshadow
	chmod 440 $(ROMFSDIR)/etc/sudoers
	chmod 755 $(ROMFSDIR)/etc/zcip.script
	chmod 755 $(ROMFSDIR)/usr/share/udhcpc/default.script
# avoid trying to chmod any symlinks
	find $(ROMFSDIR)/etc/init.d -type f -exec chmod 755 {} +
	find $(ROMFSDIR)/usr/bin -type f -exec chmod 755 {} +

	tic -o$(ROMFSDIR)/etc/terminfo \
	    -e $(subst $(space),$(comma),$(TERMS)) $(TERMINFO)

	LS_REAL=/bin/busybox DO_CHECK_STATUS=1 $(TOOLS_DIR)/create-checkftp.sh
	$(call generate_etc_DISTNAME)
	$(call generate_initscript_link)
	$(call install_getboardinfo)
	$(call install_awl12)
	$(call install_awl13)

	$(ROMFSINST) -e CONFIG_USER_LIGHTTPD_LIGHTTPD \
		     -s /etc/config/.htpasswd-at-admin \
			/home/www-data/admin/.htpasswd

#
# multiplier of blocks and inodes for genfs_ext2.sh
#
# GENFS_EXT2_BMUL = 20 # Default=20%
# GENFS_EXT2_IMUL = 20 # Default=20%
generate_romfs_image = \
	if [ "$(CONFIG_VENDOR_GENFS_MANUAL)" = "y" ]; then \
		genext2fs --squash-uids \
			  --number-of-inodes $(CONFIG_VENDOR_GENFS_INODES) \
			  --size-in-blocks $(CONFIG_VENDOR_GENFS_BLOCKS) \
			  --root $(ROMFSDIR) \
			  --devtable ext2_devtable.txt $(ROMFSIMG); \
	else \
		$(SHELL) $(TOOLS_DIR)/genfs_ext2.sh \
			  "--squash-uids --root $(ROMFSDIR) \
			   --devtable ext2_devtable.txt $(ROMFSIMG)" \
			  > /dev/null; \
	fi

do_depmod = \
	@if [ -e $(ROMFSDIR)/lib/modules/$(LINUX_VERSION) ]; then \
		chmod 0744 $(DEPMOD_PL); \
		$(DEPMOD_PL) -F $(ROOTDIR)/$(LINUXDIR)/System.map \
			     -k $(ROOTDIR)/$(LINUXDIR)/vmlinux \
			     -b $(ROMFSDIR)/lib/modules/$(LINUX_VERSION) \
			     > /dev/null; \
	fi

add_build_stamp = \
	@printf "\000%s\000%s\000%s" \
	       $(VERSIONPKG) $(CONFIG_VENDOR) $(CONFIG_PRODUCT) >> $(1)

.PHONY: $(LINUXBIN)
$(LINUXBIN): $(ROOTDIR)/$(LINUXDIR)/vmlinux
	-cp $(ROOTDIR)/$(LINUXDIR)/arch/arm/boot/Image $@

.PHONY: image
image: $(LINUXBIN)
	$(call do_depmod)
	$(call generate_romfs_image)
	/sbin/tune2fs -U random $(ROMFSIMG)

	gzip -c $(ROMFSIMG) > $(ZROMFSIMG)
	gzip -c $(LINUXBIN) > $(ZLINUXBIN)

	$(call add_build_stamp,$(ROMFSIMG))
	$(call add_build_stamp,$(ZROMFSIMG))
	$(call add_build_stamp,$(LINUXBIN))
	$(call add_build_stamp,$(ZLINUXBIN))
	$(CKSUM) -b -o 2 $(ROMFSIMG)  >> $(ROMFSIMG)
	$(CKSUM) -b -o 2 $(ZROMFSIMG) >> $(ZROMFSIMG)
	$(CKSUM) -b -o 2 $(LINUXBIN)  >> $(LINUXBIN)
	$(CKSUM) -b -o 2 $(ZLINUXBIN) >> $(ZLINUXBIN)

