#
# Makefile -- Build instructions
#

include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
include $(ARCH_CONFIG)
include $(ROOTDIR)/version
include ../Common/Armadillo.mk

comma := ,
empty :=
space := $(empty) $(empty)

SUBDIR_y =
SUBDIR_$(CONFIG_VENDOR_SWMGR_SWMGR) += swmgr/

all:
	for i in $(SUBDIR_y) ; do $(MAKE) -C $$i || exit $? ; done

clean:
	-for i in $(SUBDIR_y) ; do [ ! -d $$i ] || $(MAKE) -C $$i clean; done

distclean: clean
	rm -f config.$(LINUXDIR) etc/DISTNAME

romfs:
	@rm -f etc/DISTNAME
	@echo "$(DIST_NAME) v$(DIST_VERSION) ($(CONFIG_VENDOR)/$(CONFIG_PRODUCT))" > etc/DISTNAME

	[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)
	for i in $(ROMFS_DIRS); do \
		[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)/$$i; \
	done

	for i in $(SUBDIR_y) ; do $(MAKE) -C $$i romfs || exit $? ; done

	$(ROMFSINST) /etc
	$(ROMFSINST) /home
	$(ROMFSINST) /usr

	LS_REAL=/bin/busybox DO_CHECK_STATUS=1 $(TOOLS_DIR)/create-checkftp.sh

	$(ROMFSINST) -s /proc/mounts /etc/mtab

	$(ROMFSINST) ../../Generic/romfs/etc/services /etc/services

	$(ROMFSINST) -s /etc/config/HOSTNAME /etc/HOSTNAME
	$(ROMFSINST) -s /etc/config/hosts /etc/hosts
	$(ROMFSINST) -s /etc/config/resolv.conf /etc/resolv.conf
	$(ROMFSINST) -s /etc/config/interfaces /etc/network/interfaces

#	Fix up permissions
	chmod 400 $(ROMFSDIR)/etc/shadow
	chmod 400 $(ROMFSDIR)/etc/gshadow
	chmod 755 $(ROMFSDIR)/etc/zcip.script
	chmod 755 $(ROMFSDIR)/usr/share/udhcpc/default.script
#	avoid trying to chmod any symlinks
	find $(ROMFSDIR)/etc/init.d -type f -exec chmod 755 {} +
	find $(ROMFSDIR)/usr/bin -type f -exec chmod 755 {} +

	$(ROMFSINST) -s /etc/init.d/udevd /etc/rc.d/S03udevd
	$(ROMFSINST) -s /etc/init.d/checkroot /etc/rc.d/S05checkroot
	$(ROMFSINST) -s /etc/init.d/checkftp /etc/rc.d/S06checkftp
	$(ROMFSINST) -s /etc/init.d/syslogd /etc/rc.d/S10syslogd
	$(ROMFSINST) -s /etc/init.d/klogd /etc/rc.d/S20klogd
ifeq ($(CONFIG_USER_FLATFSD_FLATFSD),y)
	$(ROMFSINST) -s /etc/init.d/flatfsd /etc/rc.d/S25flatfsd
endif
	$(ROMFSINST) -s /etc/init.d/hostname /etc/rc.d/S30hostname
	$(ROMFSINST) -s /etc/init.d/firewall /etc/rc.d/S30firewall
	$(ROMFSINST) -s /etc/init.d/networking /etc/rc.d/S40networking

	[ "$(CONFIG_USER_INETD_INETD)" != "y" ] && \
	[ "$(CONFIG_USER_BUSYBOX_INETD)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/inetd /etc/rc.d/S60inetd

	[ "$(CONFIG_USER_THTTPD_THTTPD)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/thttpd /etc/rc.d/S70thttpd

	[ "$(CONFIG_USER_LIGHTTPD_LIGHTTPD)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/lighttpd /etc/rc.d/S70lighttpd

	[ "$(CONFIG_USER_UCDSNMP_SNMPD)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/snmpd /etc/rc.d/S70snmpd

	[ "$(CONFIG_USER_FLATFSD_FLATFSD)" != "y" ] || \
	[ "$(CONFIG_USER_OPENSSH_SSHD)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/sshd /etc/rc.d/S70sshd

	$(ROMFSINST) -s /etc/init.d/misc /etc/rc.d/S99misc
	[ "$(CONFIG_USER_FLATFSD_FLATFSD)" != "y" ] || \
	$(ROMFSINST) -s /etc/init.d/rc.local /etc/rc.d/S99rc.local

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

