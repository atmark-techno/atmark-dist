#
# Makefile -- Build instructions for NetSilicon AT-9750LX board
#


include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
include $(ARCH_CONFIG)
include $(ROOTDIR)/version

LINUXBIN  = $(IMAGEDIR)/linux.bin
ZLINUXBIN = $(IMAGEDIR)/linux.bin.gz
ROMFSIMG  = $(IMAGEDIR)/romfs.img
ZROMFSIMG = $(IMAGEDIR)/romfs.img.gz
CKSUM     = $(ROOTDIR)/tools/cksum
IMAGE     = $(ROMFSIMG)

DIRS = 

ROMFS_DIRS = root bin lib mnt tmp proc \
             etc etc/rc.d etc/dhcpc etc/terminfo etc/network \
	     etc/network/if-down.d etc/network/if-post-down.d \
	     etc/network/if-up.d etc/network/if-pre-up.d \
             dev dev/flash \
             usr usr/lib \
             var var/log var/run \
	     home home/ftp home/ftp/etc home/ftp/bin \
	     home/ftp/lib home/ftp/pub \
	     home/www-data home/www-data/cgi-bin \
	     home/guest

CROSSLIB_DIR = /usr/arm-linux/lib

TERMS = Eterm-color Eterm ansi cons25 cygwin \
        dumb hurd linux mach mach-bold mach-color pcansi \
        rxvt rxvt-basic rxvt-m \
        screen screen-bce screen-s screen-w sun \
        vt100 vt102 vt220 vt52 \
        wsvt25 wsvt25m \
        xterm xterm-color xterm-debian xterm-mono xterm-r5 xterm-r6 \
        xterm-vt220 xterm-xfree86

TERMINFO = $(ROOTDIR)/user/terminfo/termtypes.ti

comma := ,
empty :=
space := $(empty) $(empty)


all:
	dirs=$(DIRS) ; \
	for i in $$dirs ; do  $(MAKE) -C $$i || exit $? ; done

clean:
	-dirs=$(DIRS) ; \
	for i in $$dirs; do [ ! -d $$i ] || $(MAKE) -C $$i clean; done

distclean: clean
	rm -f config.$(LINUXDIR) etc/DISTNAME

romfs:
	@rm -f etc/DISTNAME
	@echo "$(DIST_NAME) v$(DIST_VERSION) ($(CONFIG_VENDOR)/$(CONFIG_PRODUCT))" > etc/DISTNAME
	[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)
	for i in $(ROMFS_DIRS); do \
		[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)/$$i; \
	done
	dirs=$(DIRS) ; \
	for i in $$dirs; do $(MAKE) -C $$i romfs || exit $? ; done

	$(ROMFSINST) /etc
	$(ROMFSINST) /home

	$(ROMFSINST) -s /proc/kcore /dev/core
	$(ROMFSINST) -s /proc/self/fd /dev/fd

	$(ROMFSINST) ../../Generic/romfs/etc/services /etc/services

#	Fix up permissions
	chmod 400 $(ROMFSDIR)/etc/shadow
	chmod 400 $(ROMFSDIR)/etc/gshadow
	chmod 400 $(ROMFSDIR)/etc/sudoers
	chmod 755 $(ROMFSDIR)/etc/init.d/*

	$(ROMFSINST) -s /etc/init.d/checkroot /etc/rc.d/S05checkroot
#	$(ROMFSINST) -s /etc/init.d/syslogd /etc/rc.d/S10syslogd
#	$(ROMFSINST) -s /etc/init.d/klogd /etc/rc.d/S20klogd
	$(ROMFSINST) -s /etc/init.d/firewall /etc/rc.d/S30firewall
	$(ROMFSINST) -s /etc/init.d/networking /etc/rc.d/S40networking
	$(ROMFSINST) -s /etc/init.d/thttpd /etc/rc.d/S50thttpd
	$(ROMFSINST) -s /etc/init.d/inetd /etc/rc.d/S60inetd

	tic -o$(ROMFSDIR)/etc/terminfo -e $(subst $(space),$(comma),$(TERMS)) $(TERMINFO)

$(LINUXBIN): $(ROOTDIR)/$(LINUXDIR)/linux
	-$(CROSS)objcopy -O binary $(ROOTDIR)/$(LINUXDIR)/linux $@

.PHONY: image
image: $(LINUXBIN)
ifdef CONFIG_VENDOR_GENFS_MANUAL
	genext2fs -U -i $(CONFIG_VENDOR_GENFS_INODES) -b $(CONFIG_VENDOR_GENFS_BLOCKS) -d $(ROMFSDIR) -D ext2_devtable.txt $(ROMFSIMG)
else #CONFIG_VENDOR_GENFS_AUTO
	@$(SHELL) $(ROOTDIR)/vendors/$(CONFIG_VENDOR)/Common/tools/genfs_ext2.sh "-U -d $(ROMFSDIR) -D ext2_devtable.txt $(ROMFSIMG)"
endif #CONFIG_VENDOR_GENFS_*
	/sbin/fsck.ext2 $(ROMFSIMG)
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

