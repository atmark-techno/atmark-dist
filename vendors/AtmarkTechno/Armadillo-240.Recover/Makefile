#
# Makefile -- Build instructions for Atmark Techno Armadillo-240.Recover board
#

UCLINUX_BUILD_USER=1
include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
include $(ARCH_CONFIG)
include $(ROOTDIR)/version
include ../Common/Armadillo.mk

ROMFS_DIRS += etc/usbmgr home/www-data/storage

CROSSLIB_DIR = /usr/arm-linux/lib

comma := ,
empty :=
space := $(empty) $(empty)

SUBDIR_y = 

SUBDIR_$(CONFIG_VENDOR_GPIOCTRL_GPIOCTRL) += ../Armadillo-2x0.Common/gpioctrl/
SUBDIR_$(CONFIG_VENDOR_LEDCTRL_LEDCTRL) += ../Armadillo-2x0.Common/ledctrl/
SUBDIR_$(CONFIG_VENDOR_SWMGR_SWMGR) += ../Armadillo-2x0.Common/swmgr/
SUBDIR_$(CONFIG_VENDOR_SLIDESHOW_SLIDESHOW) += slideshow/

all:
	for i in $(SUBDIR_y) ; do $(MAKE) -C $$i || exit $? ; done

clean:
	-for i in $(SUBDIR_y) ; do [ ! -d $$i ] || $(MAKE) -C $$i clean; done

distclean: clean
	rm -f config.$(LINUXDIR) etc/DISTNAME etc/IMAGEVERSION

romfs:
	@rm -f etc/DISTNAME etc/IMAGEVERSION
	@echo "$(DIST_NAME) v$(DIST_VERSION) ($(CONFIG_VENDOR)/$(CONFIG_PRODUCT))" > etc/DISTNAME
	@echo "$(CONFIG_VENDOR_A240_TYPE) v$(CONFIG_VENDOR_A240_VERSION)" > etc/IMAGEVERSION

	[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)
	for i in $(ROMFS_DIRS); do \
		[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)/$$i; \
	done

	for i in $(SUBDIR_y) ; do $(MAKE) -C $$i romfs || exit $? ; done

	$(ROMFSINST) /etc
	$(ROMFSINST) /usr

	LS_REAL=/bin/busybox $(TOOLS_DIR)/create-checkftp.sh

	$(ROMFSINST) -s /proc/mounts /etc/mtab
	$(ROMFSINST) -s /proc/kcore /dev/core
	$(ROMFSINST) -s /proc/self/fd /dev/fd

	$(ROMFSINST) ../../Generic/romfs/etc/services /etc/services

#	Fix up permissions
	chmod 400 $(ROMFSDIR)/etc/shadow
	chmod 400 $(ROMFSDIR)/etc/gshadow
	chmod 400 $(ROMFSDIR)/etc/sudoers
	chmod 755 $(ROMFSDIR)/etc/zcip.script
	chmod 755 $(ROMFSDIR)/usr/share/udhcpc/default.script
	chmod 744 $(ROMFSDIR)/usr/share/slideshow/mount_usbdev
	chmod 744 $(ROMFSDIR)/usr/share/slideshow/slideshow_auto_run
#	avoid trying to chmod any symlinks
	find $(ROMFSDIR)/etc/init.d -type f -exec chmod 755 {} +

	$(ROMFSINST) -s /etc/config/HOSTNAME /etc/HOSTNAME
	$(ROMFSINST) -s /etc/config/resolv.conf /etc/resolv.conf
	$(ROMFSINST) -s /etc/config/.htpasswd-at-admin /home/www-data/admin/.htpasswd

	tic -o$(ROMFSDIR)/etc/terminfo -e $(subst $(space),$(comma),$(TERMS)) $(TERMINFO)

$(LINUXBIN): $(ROOTDIR)/$(LINUXDIR)/vmlinux
ifeq ($(LINUXDIR),linux-2.6.x)
	-cp $(ROOTDIR)/$(LINUXDIR)/arch/arm/boot/Image $@
else
	-$(CROSS)objcopy -O binary $(ROOTDIR)/$(LINUXDIR)/linux $@
endif

.PHONY: image
image: $(LINUXBIN)
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

