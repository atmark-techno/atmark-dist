#
# Makefile -- Build instructions
#

include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
include $(ARCH_CONFIG)
ifeq ($(CONFIG_USER_BUSYBOX_1_20_2),y)
-include $(ROOTDIR)/user/busybox/busybox-1.20.2/.config
endif
include $(ROOTDIR)/version
include Armadillo.mk

DEPMOD_PL = ../Common/tools/depmod.pl
PREBUILD_LIBDIR = $(ROOTDIR)/lib/prebuild/$(CROSS_COMPILE:-=)
PREBUILD_PRODUCTDIR = prebuild/$(CROSS_COMPILE:-=)

GCONV_FILES = gconv-modules ISO8859-1.so

ROOT_PASSWD := \
	$(shell perl $(ROOTDIR)/tools/crypt.pl $(CONFIG_VENDOR_ROOT_PASSWD))

update_root_passward = \
	chmod 644 $(ROMFSDIR)/etc/shadow; \
	sed -i -e "s/^root:.*//" -e "/^$$/d" $(ROMFSDIR)/etc/shadow; \
	$(ROMFSINST) -A "root" -a "root:${ROOT_PASSWD}:1:0:99999:7:::" \
		/etc/shadow

comma := ,
empty :=
space := $(empty) $(empty)

SUBDIR_y =

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
	$(ROMFSINST) /usr

	$(ROMFSINST) -s /proc/mounts /etc/mtab

	$(ROMFSINST) ../../Generic/romfs/etc/services /etc/services

	$(ROMFSINST) -s /etc/config/HOSTNAME /etc/HOSTNAME
	$(ROMFSINST) -s /etc/config/hosts /etc/hosts
	$(ROMFSINST) -s /etc/config/resolv.conf /etc/resolv.conf
	$(ROMFSINST) -s /etc/config/interfaces /etc/network/interfaces

#	Fix up permissions
	$(call update_root_passward)
	chmod 400 $(ROMFSDIR)/etc/shadow
	chmod 400 $(ROMFSDIR)/etc/gshadow
	chmod 440 $(ROMFSDIR)/etc/sudoers
	chmod 755 $(ROMFSDIR)/usr/share/udhcpc/default.script
#	avoid trying to chmod any symlinks
	find $(ROMFSDIR)/etc/init.d -type f -exec chmod 755 {} +
	find $(ROMFSDIR)/usr/bin -type f -exec chmod 755 {} +

ifeq ($(CONFIG_USER_UDEV),y)
	$(ROMFSINST) -s /etc/init.d/udev /etc/rc.d/S03udev
endif
ifeq ($(CONFIG_USER_FLATFSD_FLATFSD),y)
	$(ROMFSINST) -s /etc/init.d/flatfsd /etc/rc.d/S04flatfsd
endif
	$(ROMFSINST) -s /etc/init.d/checkroot /etc/rc.d/S05checkroot
	$(ROMFSINST) -s /etc/init.d/mountdevsubfs /etc/rc.d/S06mountdevsubfs
	$(ROMFSINST) -s /etc/init.d/syslogd /etc/rc.d/S10syslogd
	$(ROMFSINST) -s /etc/init.d/klogd /etc/rc.d/S20klogd
	$(ROMFSINST) -s /etc/init.d/module-init-tools /etc/rc.d/S25module-init-tools
	$(ROMFSINST) -s /etc/init.d/hostname /etc/rc.d/S30hostname
	$(ROMFSINST) -s /etc/init.d/firewall /etc/rc.d/S30firewall
	$(ROMFSINST) -s /etc/init.d/networking /etc/rc.d/S40networking

	[ "$(CONFIG_USER_AVAHI_AVAHI)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/avahi /etc/rc.d/S71avahi

	[ "$(CONFIG_USER_FLATFSD_FLATFSD)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/rc.local /etc/rc.d/S99rc.local

	tic -o$(ROMFSDIR)/etc/terminfo -e $(subst $(space),$(comma),$(TERMS)) $(TERMINFO)

	$(ROMFSINST) $(PREBUILD_PRODUCTDIR)/usr/sbin/get-board-info-abws1 \
		     /usr/sbin/get-board-info-abws1
	$(ROMFSINST) -s /usr/sbin/get-board-info-abws1 /usr/sbin/get-board-info

.PHONY: $(LINUXBIN)
$(LINUXBIN): $(ROOTDIR)/$(LINUXDIR)/vmlinux
	-cp $(ROOTDIR)/$(LINUXDIR)/arch/arm/boot/Image $@

#
# multiplier of blocks and inodes for genfs_ext2.sh
#
# GENFS_EXT2_BMUL = 20 # Default=20%
# GENFS_EXT2_IMUL = 20 # Default=20%

do_depmod = \
	@if [ -e $(ROMFSDIR)/lib/modules/$(LINUX_VERSION) ]; then \
		chmod 0744 $(DEPMOD_PL); \
		$(DEPMOD_PL) -F $(ROOTDIR)/$(LINUXDIR)/System.map \
			     -k $(ROOTDIR)/$(LINUXDIR)/vmlinux \
			     -b $(ROMFSDIR)/lib/modules/$(LINUX_VERSION) \
			     > /dev/null; \
	fi

.PHONY: image
image: $(LINUXBIN)
	$(call do_depmod)
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
