#
# Makefile -- Build instructions
#

include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
include $(ARCH_CONFIG)
include $(ROOTDIR)/version
include ../Common/Armadillo.mk

DEPMOD_PL = ../Common/tools/depmod.pl
PREBUILD_LIBDIR = $(ROOTDIR)/lib/prebuild/$(CROSS_COMPILE:-=)

GCONV_FILES = gconv-modules ISO8859-1.so

comma := ,
empty :=
space := $(empty) $(empty)

SUBDIR_y =
SUBDIR_$(CONFIG_VENDOR_GPIOCTRL_GPIOCTRL)	+= gpioctrl/
SUBDIR_$(CONFIG_VENDOR_LEDCTRL_LEDCTRL)		+= ledctrl/
SUBDIR_$(CONFIG_VENDOR_SWMGR_SWMGR)		+= swmgr/
SUBDIR_$(CONFIG_VENDOR_AWL12_AERIAL)		+= awl12/
SUBDIR_$(CONFIG_VENDOR_AWL13_AWL13)		+= awl13/

AWL13_TARGET=armadillo400
AWL13_SDIO_FWIMAGE_y =
AWL13_SDIO_FWIMAGE_$(CONFIG_VENDOR_AWL13_AWL13_STA)	+= $(shell ls -1v ../Armadillo-WLAN.Common/lib/firmware/awl13/fwimage*_STA_SDIO.bin | tail -1)
AWL13_SDIO_FWIMAGE_$(CONFIG_VENDOR_AWL13_AWL13_AP)	+= $(shell ls -1v ../Armadillo-WLAN.Common/lib/firmware/awl13/fwimage*_AP_SDIO.bin | tail -1)
AWL13_USB_FWIMAGE_y =
AWL13_USB_FWIMAGE_$(CONFIG_VENDOR_AWL13_AWL13_STA)	+= $(shell ls -1v ../Armadillo-WLAN.Common/lib/firmware/awl13/fwimage*_STA_USB.bin | tail -1)
AWL13_USB_FWIMAGE_$(CONFIG_VENDOR_AWL13_AWL13_AP)	+= $(shell ls -1v ../Armadillo-WLAN.Common/lib/firmware/awl13/fwimage*_AP_USB.bin | tail -1)

all:
	for i in $(SUBDIR_y) ; do $(MAKE) -C $$i || exit $? ; done

clean:
	-for i in $(SUBDIR_y) ; do [ ! -d $$i ] || $(MAKE) -C $$i clean; done

distclean: clean
	rm -f config.$(LINUXDIR) etc/DISTNAME

romfs:
	@rm -f etc/DISTNAME
	@echo "$(DIST_NAME) v$(DIST_VERSION) ($(CONFIG_VENDOR)/$(CONFIG_PRODUCT))" > etc/DISTNAME

	mkdir -p $(ROMFSDIR)
	for i in $(ROMFS_DIRS); do \
		[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)/$$i; \
	done

	for i in $(SUBDIR_y) ; do $(MAKE) -C $$i romfs || exit $? ; done

	$(ROMFSINST) /etc
	$(ROMFSINST) /home
	$(ROMFSINST) /usr

ifeq ($(CONFIG_USER_XSERVER_KDRIVE),y)
	cp -a $(PREBUILD_LIBDIR)/ts/ $(PREBUILD_LIBDIR)/pango/ \
	      $(ROMFSDIR)/usr/lib/
	mkdir -p $(ROMFSDIR)/usr/lib/gconv
	cp $(addprefix $(PREBUILD_LIBDIR)/gconv/,$(GCONV_FILES)) \
	   $(ROMFSDIR)/usr/lib/gconv
endif

	LS_REAL=/bin/busybox DO_CHECK_STATUS=1 $(TOOLS_DIR)/create-checkftp.sh

	$(ROMFSINST) -s /proc/mounts /etc/mtab

	$(ROMFSINST) ../../Generic/romfs/etc/services /etc/services

	$(ROMFSINST) -s /etc/config/HOSTNAME /etc/HOSTNAME
	$(ROMFSINST) -s /etc/config/hosts /etc/hosts
	$(ROMFSINST) -s /etc/config/pointercal /etc/pointercal
	$(ROMFSINST) -s /etc/config/resolv.conf /etc/resolv.conf
	$(ROMFSINST) -s /etc/config/interfaces /etc/network/interfaces

#	Fix up permissions
	chmod 400 $(ROMFSDIR)/etc/shadow
	chmod 400 $(ROMFSDIR)/etc/gshadow
	chmod 440 $(ROMFSDIR)/etc/sudoers
	chmod 755 $(ROMFSDIR)/etc/zcip.script
	chmod 755 $(ROMFSDIR)/usr/share/udhcpc/default.script
#	avoid trying to chmod any symlinks
	find $(ROMFSDIR)/etc/init.d -type f -exec chmod 755 {} +
	find $(ROMFSDIR)/usr/bin -type f -exec chmod 755 {} +

ifeq ($(CONFIG_USER_UDEV),y)
	$(ROMFSINST) -s /etc/init.d/udevd /etc/rc.d/S03udevd
endif
ifeq ($(CONFIG_USER_FLATFSD_FLATFSD),y)
	$(ROMFSINST) -s /etc/init.d/flatfsd /etc/rc.d/S04flatfsd
endif
	$(ROMFSINST) -s /etc/init.d/checkroot /etc/rc.d/S05checkroot
	$(ROMFSINST) -s /etc/init.d/checkftp /etc/rc.d/S06checkftp
	$(ROMFSINST) -s /etc/init.d/syslogd /etc/rc.d/S10syslogd
	$(ROMFSINST) -s /etc/init.d/klogd /etc/rc.d/S20klogd
	$(ROMFSINST) -s /etc/init.d/module-init-tools /etc/rc.d/S25module-init-tools
	$(ROMFSINST) -s /etc/init.d/hostname /etc/rc.d/S30hostname
	$(ROMFSINST) -s /etc/init.d/firewall /etc/rc.d/S30firewall
	$(ROMFSINST) -s /etc/init.d/networking /etc/rc.d/S40networking

	[ "$(CONFIG_USER_INETD_INETD)" != "y" ] && \
	[ "$(CONFIG_USER_BUSYBOX_INETD)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/inetd /etc/rc.d/S60inetd

	[ "$(CONFIG_USER_AT_CGI_SYSTEM)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/at-cgi /etc/rc.d/S70at-cgi

ifeq ($(CONFIG_USER_LIGHTTPD_LIGHTTPD),y)
	$(ROMFSINST) -s /etc/init.d/lighttpd /etc/rc.d/S70lighttpd
	$(ROMFSINST) -s /etc/config/.htpasswd-at-admin \
			/home/www-data/admin/.htpasswd
endif

	[ "$(CONFIG_USER_UCDSNMP_SNMPD)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/snmpd /etc/rc.d/S70snmpd

	[ "$(CONFIG_USER_AVAHI_AVAHI)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/avahi /etc/rc.d/S71avahi

	[ "$(CONFIG_USER_XSERVER_KDRIVE)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/xfbdev /etc/rc.d/S80xfbdev

	$(ROMFSINST) -s /etc/init.d/misc /etc/rc.d/S99misc
	[ "$(CONFIG_USER_FLATFSD_FLATFSD)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/rc.local /etc/rc.d/S99rc.local

	tic -o$(ROMFSDIR)/etc/terminfo -e $(subst $(space),$(comma),$(TERMS)) $(TERMINFO)

ifeq ($(CONFIG_VENDOR_AWL12_AERIAL),y)
	$(ROMFSINST) ../Armadillo-WLAN.Common/lib/firmware/aerial /lib/firmware/aerial
	[ `grep -x "aerial" $(ROMFSDIR)/etc/modules > /dev/null` ] || \
	echo "aerial" >> $(ROMFSDIR)/etc/modules
	$(ROMFSINST) ../Armadillo-WLAN.Common/etc/default/aerial-firmware-load.sh /etc/default/aerial-firmware-load.sh
	$(ROMFSINST) ../Armadillo-WLAN.Common/etc/udev/rules.d/z05_aerial.rules /etc/udev/rules.d/z05_aerial.rules
endif

ifeq ($(CONFIG_VENDOR_AWL13_AWL13),y)
	rm -rf $(ROMFSDIR)/lib/firmware/awl13
	mkdir -p $(ROMFSDIR)/lib/firmware/awl13
ifeq ($(CONFIG_VENDOR_AWL13_AWL13_SDIO),y)
	$(ROMFSINST) $(AWL13_SDIO_FWIMAGE_y) /lib/firmware/awl13/$(notdir $(AWL13_SDIO_FWIMAGE_y))
	[ `grep -x "awl13_sdio" $(ROMFSDIR)/etc/modules > /dev/null` ] || \
	echo "awl13_sdio" >> $(ROMFSDIR)/etc/modules
endif
ifeq ($(CONFIG_VENDOR_AWL13_AWL13_USB),y)
	$(ROMFSINST) $(AWL13_USB_FWIMAGE_y) /lib/firmware/awl13/$(notdir $(AWL13_USB_FWIMAGE_y))
	[ `grep -x "awl13_usb" $(ROMFSDIR)/etc/modules > /dev/null` ] || \
	echo "awl13_usb" >> $(ROMFSDIR)/etc/modules
endif
	$(ROMFSINST) ../Armadillo-WLAN.Common/etc/default/awl13-firmware-load.sh /etc/default/awl13-firmware-load.sh
	$(ROMFSINST) ../Armadillo-WLAN.Common/etc/udev/rules.d/z05_awl13.rules /etc/udev/rules.d/z05_awl13.rules
endif

.PHONY: $(LINUXBIN)
$(LINUXBIN): $(ROOTDIR)/$(LINUXDIR)/vmlinux
ifeq ($(LINUXDIR),linux-2.6.x)
	-cp $(ROOTDIR)/$(LINUXDIR)/arch/arm/boot/Image $@
else
	-$(CROSS)objcopy -O binary $(ROOTDIR)/$(LINUXDIR)/linux $@
endif

#
# multiplier of blocks and inodes for genfs_ext2.sh
#
# GENFS_EXT2_BMUL = 20 # Default=20%
# GENFS_EXT2_IMUL = 20 # Default=20%

.PHONY: image
image: $(LINUXBIN)
ifeq ($(LINUXDIR),linux-2.6.x)
	@chmod 0744 $(DEPMOD_PL)
	$(DEPMOD_PL) -F $(ROOTDIR)/$(LINUXDIR)/System.map -k $(ROOTDIR)/$(LINUXDIR)/vmlinux -b $(ROMFSDIR)/lib/modules/2.6.* &> /dev/null
endif
ifdef CONFIG_VENDOR_GENFS_MANUAL
	genext2fs --squash-uids --number-of-inodes $(CONFIG_VENDOR_GENFS_INODES) --size-in-blocks $(CONFIG_VENDOR_GENFS_BLOCKS) --root $(ROMFSDIR) --devtable ext2_devtable.txt $(ROMFSIMG)
else #CONFIG_VENDOR_GENFS_AUTO
	@$(SHELL) $(TOOLS_DIR)/genfs_ext2.sh "--squash-uids --root $(ROMFSDIR) --devtable ext2_devtable.txt $(ROMFSIMG)"
endif #CONFIG_VENDOR_GENFS_*
	/sbin/tune2fs -U random $(ROMFSIMG)
	gzip -c $(ROMFSIMG) > $(ZROMFSIMG)
	gzip -c $(LINUXBIN) > $(ZLINUXBIN)
	echo -ne "\000"           | tee -a $(ROMFSIMG) | tee -a $(ZROMFSIMG) | tee -a $(LINUXBIN) >> $(ZLINUXBIN)
	echo -n $(VERSIONPKG)     | tee -a $(ROMFSIMG) | tee -a $(ZROMFSIMG) | tee -a $(LINUXBIN) >> $(ZLINUXBIN)
	echo -ne "\000"           | tee -a $(ROMFSIMG) | tee -a $(ZROMFSIMG) | tee -a $(LINUXBIN) >> $(ZLINUXBIN)
	echo -n $(CONFIG_VENDOR)  | tee -a $(ROMFSIMG) | tee -a $(ZROMFSIMG) | tee -a $(LINUXBIN) >> $(ZLINUXBIN)
	echo -ne "\000"           | tee -a $(ROMFSIMG) | tee -a $(ZROMFSIMG) | tee -a $(LINUXBIN) >> $(ZLINUXBIN)
	echo -n $(CONFIG_PRODUCT) | tee -a $(ROMFSIMG) | tee -a $(ZROMFSIMG) | tee -a $(LINUXBIN) >> $(ZLINUXBIN)
	$(CKSUM) -b -o 2 $(ROMFSIMG)  >> $(ROMFSIMG)
	$(CKSUM) -b -o 2 $(ZROMFSIMG) >> $(ZROMFSIMG)
	$(CKSUM) -b -o 2 $(LINUXBIN)  >> $(LINUXBIN)
	$(CKSUM) -b -o 2 $(ZLINUXBIN) >> $(ZLINUXBIN)

