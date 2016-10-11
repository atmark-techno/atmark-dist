#
# Makefile -- Build instructions for Atmark Techno Armadillo-210.Recover board
#

UCLINUX_BUILD_USER=1
include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
include $(ARCH_CONFIG)
include $(ROOTDIR)/version
include ../Common/Armadillo.mk

CROSSLIB_DIR = /usr/arm-linux/lib

comma := ,
empty :=
space := $(empty) $(empty)

SUBDIR_y = 

SUBDIR_$(CONFIG_VENDOR_GPIOCTRL_GPIOCTRL) += ../Armadillo-210.Common/gpioctrl/
SUBDIR_$(CONFIG_VENDOR_LEDCTRL_LEDCTRL) += ../Armadillo-210.Common/ledctrl/

all:
	for i in $(SUBDIR_y) ; do $(MAKE) -C $$i || exit $? ; done

clean:
	-for i in $(SUBDIR_y) ; do [ ! -d $$i ] || $(MAKE) -C $$i clean; done
	rm -f etc/IMAGEVERSION

distclean: clean
	rm -f config.$(LINUXDIR) etc/DISTNAME

romfs:
	@rm -f etc/DISTNAME etc/IMAGEVERSION
	@echo "$(DIST_NAME) v$(DIST_VERSION) ($(CONFIG_VENDOR)/$(CONFIG_PRODUCT))" > etc/DISTNAME
	@echo "$(CONFIG_VENDOR_A210_TYPE) v$(CONFIG_VENDOR_A210_VERSION)" > etc/IMAGEVERSION

	[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)
	for i in $(ROMFS_DIRS); do \
		[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)/$$i; \
	done

	for i in $(SUBDIR_y) ; do $(MAKE) -C $$i romfs || exit $? ; done

	$(ROMFSINST) /etc
	$(ROMFSINST) /home

	LS_REAL=/bin/busybox $(TOOLS_DIR)/create-checkftp.sh

	$(ROMFSINST) -s /proc/mounts /etc/mtab
	$(ROMFSINST) -s /proc/kcore /dev/core
	$(ROMFSINST) -s /proc/self/fd /dev/fd

	$(ROMFSINST) ../../Generic/romfs/etc/services /etc/services

#	Fix up permissions
	chmod 400 $(ROMFSDIR)/etc/shadow
	chmod 400 $(ROMFSDIR)/etc/gshadow
#	avoid trying to chmod any symlinks
	find $(ROMFSDIR)/etc/init.d -type f -exec chmod 755 {} +

	$(ROMFSINST) -s /etc/config/resolv.conf /etc/resolv.conf

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

