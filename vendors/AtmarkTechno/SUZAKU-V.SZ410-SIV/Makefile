#
# Makefile -- Build instructions for Atmark Techno SUZAKU board
#

include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
include $(ARCH_CONFIG)

LINUXBIN = $(IMAGEDIR)/linux.bin
ROMFSIMG = $(IMAGEDIR)/romfs.img
IMAGE    = $(IMAGEDIR)/image.bin
ELFIMAGE = $(IMAGEDIR)/image.elf
TOOLS    = $(ROOTDIR)/tools

ROMFS_DIRS = bin dev dev/flash etc home lib mnt proc sbin usr var

ifdef CONFIG_USER_FLATFSD_FLATFSD
ETCDIR = /etc/default
else
ETCDIR = /etc
endif

DEVICES =								      \
	tty,c,5,0      console,c,5,1   mem,c,1,1     kmem,c,1,2    null,c,1,3 \
									      \
	ttyS0,c,4,64							      \
									      \
	zero,c,1,5     random,c,1,8    urandom,c,1,9			      \
									      \
	ptyp0,c,2,0    ptyp1,c,2,1     ptyp2,c,2,2   ptyp3,c,2,3	      \
	ptyp4,c,2,4    ptyp5,c,2,5     ptyp6,c,2,6   ptyp7,c,2,7	      \
	ptyp8,c,2,8    ptyp9,c,2,9     ptypa,c,2,10  ptypb,c,2,11	      \
	ptypc,c,2,12   ptypd,c,2,13    ptype,c,2,14  ptypf,c,2,15	      \
									      \
	ttyp0,c,3,0    ttyp1,c,3,1     ttyp2,c,3,2   ttyp3,c,3,3	      \
	ttyp4,c,3,4    ttyp5,c,3,5     ttyp6,c,3,6   ttyp7,c,3,7	      \
	ttyp8,c,3,8    ttyp9,c,3,9     ttypa,c,3,10  ttypb,c,3,11	      \
	ttypc,c,3,12   ttypd,c,3,13    ttype,c,3,14  ttypf,c,3,15	   

DEVICES +=   \
	mtd0,c,90,0	mtdr0,c,90,1	mtdblock0,b,31,0	\
	mtd1,c,90,2	mtdr1,c,90,3	mtdblock1,b,31,1	\
	mtd2,c,90,4	mtdr2,c,90,5	mtdblock2,b,31,2	\
	mtd3,c,90,6	mtdr3,c,90,7	mtdblock3,b,31,3	\
	mtd4,c,90,8 	mtdr4,c,90,9	mtdblock4,b,31,4	\
	mtd5,c,90,10 	mtdr5,c,90,11	mtdblock5,b,31,5	\
	mtd6,c,90,12 	mtdr6,c,90,13	mtdblock6,b,31,6	\
	mtd7,c,90,14 	mtdr7,c,90,15	mtdblock7,b,31,7

ifdef CONFIG_SUZAKU_STARTERKIT_7SEG_CGI
DEVICES +=	\
	sil7segc,c,65,0
endif

ifdef CONFIG_SUZAKU_SIL_LED
DEVICES +=	\
	silled,c,70,0	\
	silled1,c,70,1	silled2,c,70,2	silled3,c,70,3	silled4,c,70,4
endif

ifdef CONFIG_SUZAKU_SIL_7SEG
DEVICES +=	\
	sil7seg,c,71,0	\
	sil7seg1,c,71,1	sil7seg2,c,71,2	sil7seg3,c,71,3
endif

ifdef CONFIG_SUZAKU_SIL_SW
DEVICES +=	\
	silsw,c,72,0	\
	silsw1,c,72,1	silsw2,c,72,2	silsw3,c,72,3
endif

ifdef CONFIG_SUZAKU_SIL_RSW
DEVICES +=	\
	silrsw,c,73,0
endif

ifdef CONFIG_SUZAKU_SIL_RS232C
DEVICES +=	\
	ttyS1,c,4,65
endif

ifdef CONFIG_SUZAKU_SID
DEVICES +=	\
	sid,c,74,0
endif

ifdef CONFIG_SUZAKU_SIV
DEVICES +=	\
        fb0,c,29,0     fb1,c,29,1      fb2,c,29,2
endif

ifdef CONFIG_SUZAKU_SIA
DEVICES +=	\
        mixer,c,14,0   dsp,c,14,3     audio,c,14,4

ROMFS_DIRS +=	\
	dev/snd

SND_DEVICES =			\
	controlC0,c,116,0	\
	pcmC0D0p,c,116,16	\
	pcmC0D0c,c,116,24	\
	timer,c,116,33
endif

FLASH_DEVICES = 	\
	all,c,90,0	\
	fpga,c,90,2	\
	boot,c,90,4	\
	config,c,90,6	\
	image,c,90,8	\
	kernel,c,90,10	\
	user,c,90,12

ROOT_PASSWD := $(shell perl $(TOOLS)/crypt.pl $(CONFIG_VENDOR_SUZAKU_ROOT_PASSWD))

INETD_CONF=/etc/inetd.conf

SUBDIR_y =

SUBDIR_$(CONFIG_VENDOR_SUZAKU_SIL_LED)	+= ../SUZAKU-V.Common/sil
SUBDIR_$(CONFIG_VENDOR_SUZAKU_SIL_7SEG)	+= ../SUZAKU-V.Common/sil
SUBDIR_$(CONFIG_VENDOR_SUZAKU_SIL_SW)	+= ../SUZAKU-V.Common/sil
SUBDIR_$(CONFIG_VENDOR_SUZAKU_SIL_RSW)	+= ../SUZAKU-V.Common/sil
SUBDIR_$(CONFIG_VENDOR_SUZAKU_STARTERKIT_7SEG_CGI) += ../SUZAKU-V.Common/starterkit
SUBDIR_$(CONFIG_VENDOR_SUZAKU_SID_AD)	+= ../SUZAKU-V.Common/sid
SUBDIR_$(CONFIG_VENDOR_SUZAKU_SIV_AV)	+= ../SUZAKU-V.Common/siv
SUBDIR_$(CONFIG_VENDOR_SUZAKU_SIV_IMPOSE) += ../SUZAKU-V.Common/siv

all:
	for i in $(SUBDIR_y) ; do $(MAKE) -C $$i || exit $? ; done

clean:
	-for i in $(SUBDIR_y) ; do [ ! -d $$i ] || $(MAKE) -C $$i clean; done

distclean: clean
	rm -f config.$(LINUXDIR) config.$(LINUXDIR)

romfs:
	[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)
	for i in $(ROMFS_DIRS); do \
		[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)/$$i; \
	done

	for i in $(SUBDIR_y) ; do $(MAKE) -C $$i romfs || exit $? ; done

	mkdir -p $(ROMFSDIR)/etc/init.d
	mkdir -p $(ROMFSDIR)/etc/rc.d

	$(ROMFSINST) -s /proc/mounts /etc/mtab

ifeq ($(findstring uClibc,$(LIBCDIR)),uClibc)
	$(ROMFSINST) -s ld-uClibc-0.9.29.so /lib/ld.so.1
endif

ifdef CONFIG_MTD_ATTACHED_ROMFS
	for i in $(DEVICES); do \
		touch $(ROMFSDIR)/dev/@$$i; \
	done
	for i in $(FLASH_DEVICES); do \
		touch $(ROMFSDIR)/dev/flash/@$$i; \
	done
	for i in $(SND_DEVICES); do \
		touch $(ROMFSDIR)/dev/snd/@$$i; \
	done
	$(ROMFSINST) -s /var/tmp /tmp
else
	$(ROMFSINST) etc/rc/checkroot /etc/init.d/checkroot
	$(ROMFSINST) -s /etc/init.d/checkroot /etc/rc.d/S01checkroot
endif

ifdef CONFIG_USER_FLATFSD_FLATFSD
	mkdir -p $(ROMFSDIR)/etc/config
	mkdir -p $(ROMFSDIR)$(ETCDIR)
	$(ROMFSINST) etc/rc/flatfsd /etc/init.d/flatfsd
	$(ROMFSINST) -s /etc/init.d/flatfsd /etc/rc.d/S02flatfsd
endif

	$(ROMFSINST) etc/host.conf /etc/host.conf
	$(ROMFSINST) etc/hosts /etc/hosts
	$(ROMFSINST) etc/hosts.allow /etc/hosts.allow
	$(ROMFSINST) etc/hosts.deny /etc/hosts.deny
	$(ROMFSINST) etc/nsswitch.conf /etc/nsswitch.conf
	$(ROMFSINST) etc/protocols /etc/protocols
	$(ROMFSINST) etc/rpc /etc/rpc

	$(ROMFSINST) -A ^root: -a "root:${ROOT_PASSWD}:0:0::/:/bin/sh" $(ETCDIR)/passwd
	$(ROMFSINST) -A ^nobody: -a "nobody:!:65534:65534::/tmp:" $(ETCDIR)/passwd
ifdef CONFIG_USER_FLATFSD_FLATFSD
	$(ROMFSINST) -s /etc/config/passwd /etc/passwd
endif

ifdef CONFIG_USER_INIT_INIT
ifdef CONFIG_USER_TINYLOGIN_GETTY
	$(ROMFSINST) -a "ttyS0:vt100:/sbin/getty ttyS0 115200" /etc/inittab
else
	$(ROMFSINST) -a "ttyS0:vt100:/bin/sh" /etc/inittab
endif
	$(ROMFSINST) -e CONFIG_USER_FLATFSD_FLATFSD \
			-a "flt:unknown:/bin/flatfsd" /etc/inittab
	$(ROMFSINST) -e CONFIG_USER_INETD_INETD \
			-a "inet:unknown:/bin/inetd" /etc/inittab
	$(ROMFSINST) -e CONFIG_USER_FLATFSD_FLATFSD \
			etc/rc/start $(ETCDIR)/start
	$(ROMFSINST) -e CONFIG_USER_FLATFSD_FLATFSD \
			etc/inittab.config $(ETCDIR)/inittab
	$(ROMFSINST) etc/rc/sysinit /etc/rc
endif

ifdef CONFIG_USER_BUSYBOX_INIT
	$(ROMFSINST) etc/rc/sysinit /etc/rc.sysinit
	$(ROMFSINST) etc/rc/reboot /etc/rc.reboot
ifdef CONFIG_USER_TINYLOGIN_GETTY
	$(ROMFSINST) etc/inittab.getty /etc/inittab
else
	$(ROMFSINST) etc/inittab.sh /etc/inittab
endif
endif

	$(ROMFSINST) -e CONFIG_USER_NET_TOOLS_HOSTNAME \
			etc/rc/hostname /etc/init.d/hostname
	$(ROMFSINST) -e CONFIG_USER_SYSUTILS_HOSTNAME  \
			etc/rc/hostname /etc/init.d/hostname
	$(ROMFSINST) -e CONFIG_USER_BUSYBOX_HOSTNAME \
			etc/rc/hostname /etc/init.d/hostname
	-[ -e $(ROMFSDIR)/etc/init.d/hostname ] && \
		$(ROMFSINST) -a "/bin/hostname ${CONFIG_PRODUCT}" /etc/init.d/hostname
	-[ -e $(ROMFSDIR)/etc/init.d/hostname ] && \
		$(ROMFSINST) -s /etc/init.d/hostname /etc/rc.d/S30hostname

############################################

ifdef CONFIG_NET

# setup /etc/init.d/network and/or /etc/resolv.conf
ifdef CONFIG_USER_DHCPCD_NEW_DHCPCD
	$(ROMFSINST) etc/rc/dhcpcd-new /etc/init.d/network
endif
	$(ROMFSINST) etc/resolv.conf $(ETCDIR)/resolv.conf
ifdef CONFIG_USER_FLATFSD_FLATFSD
	$(ROMFSINST) -s /etc/config/resolv.conf /etc/resolv.conf
endif

# this is the special case, overwrite /etc/init.d/network
ifdef CONFIG_USER_FLATFSD_FLATFSD
	$(ROMFSINST) etc/rc/ifconfig $(ETCDIR)/network
endif
ifdef CONFIG_USER_DHCPCD_NEW_DHCPCD
ifdef CONFIG_MTD_ATTACHED_ROMFS
ifndef CONFIG_USER_FLATFSD_FLATFSD
	$(ROMFSINST) etc/rc/dhcpcd-new.no-resolv.conf /etc/init.d/network
endif
else
	mkdir -p $(ROMFSDIR)/etc/dhcpc
endif
else
ifdef CONFIG_USER_FLATFSD_FLATFSD
	echo "/bin/sh /etc/config/network" > $(ROMFSDIR)/etc/init.d/network
else
	$(ROMFSINST) etc/rc/ifconfig /etc/init.d/network
endif
endif

	$(ROMFSINST) -s /etc/init.d/network /etc/rc.d/S40network

	$(ROMFSINST) -e CONFIG_USER_INETD_INETD \
			etc/rc/inetd /etc/init.d/inetd
	$(ROMFSINST) -e CONFIG_USER_INETD_INETD \
			-s /etc/init.d/inetd /etc/rc.d/S50inetd

ifdef CONFIG_USER_INETD_INETD
	$(ROMFSINST) -e CONFIG_USER_TELNETD_TELNETD \
		-a "telnet  stream tcp nowait root /bin/telnetd" $(INETD_CONF)
	$(ROMFSINST) -e CONFIG_USER_FTPD_FTPD \
		-a "ftp     stream tcp nowait root /bin/ftpd -l" $(INETD_CONF)
endif

ifdef CONFIG_USER_THTTPD_THTTPD
	$(ROMFSINST) etc/rc/thttpd /etc/init.d/thttpd
	$(ROMFSINST) -s /etc/init.d/thttpd /etc/rc.d/S90thttpd
	$(ROMFSINST) /home/httpd
endif

ifdef CONFIG_USER_FLATFSD_FLATFSD
	$(ROMFSINST) etc/rc/rc.local /etc/init.d/rc.local
	$(ROMFSINST) -s /etc/init.d/rc.local /etc/rc.d/S99rc.local
endif

	$(ROMFSINST) ../../Generic/romfs/etc/services /etc/services
endif

# At last, change the permission of boot scripts
	chmod 754 $(ROMFSDIR)/etc/rc.sysinit
	chmod 754 $(ROMFSDIR)/etc/rc.reboot
# avoid trying to chmod any symlinks
	find $(ROMFSDIR)/etc/init.d -type f -exec chmod 755 {} +

$(LINUXBIN): $(ROOTDIR)/$(LINUXDIR)/linux
	[ -d $(IMAGEDIR) ] || mkdir -p $(IMAGEDIR)
	$(CROSS)objcopy -O binary $(ROOTDIR)/$(LINUXDIR)/linux $@

$(ROMFSIMG): $(LINUXBIN)
	genromfs -v -V "ROMdisk" -x *~ -f $(ROMFSIMG) -d $(ROMFSDIR)

.PHONY: image $(ROMFSIMG)
image: $(LINUXBIN) $(ROMFSIMG)
ifdef CONFIG_MTD_ATTACHED_ROMFS
	BSS=`$(CROSS)objdump --headers $(ROOTDIR)/$(LINUXDIR)/linux | \
      grep "\.bss"` ; \
    ADDR=`set -- $${BSS} ; echo 0x$${4}` ; \
    $(CROSS)objcopy --add-section=.romfs=$(ROMFSIMG) \
      --adjust-section-vma=.romfs=$${ADDR} --no-adjust-warnings \
      --set-section-flags=.romfs=alloc,load,data   \
      $(ROOTDIR)/$(LINUXDIR)/linux $(ELFIMAGE)
	$(CROSS)objcopy -O binary --remove-section=.stab --remove-section=.stabstr $(ELFIMAGE) $(IMAGE)
ifdef CONFIG_USER_NETFLASH_VERSION
	echo -ne "\000" >> $(IMAGE)
	echo -n $(VERSIONPKG) >> $(IMAGE)
	echo -ne "\000" >> $(IMAGE)
	echo -n $(CONFIG_VENDOR) >> $(IMAGE)
	echo -ne "\000" >> $(IMAGE)
	echo -n $(CONFIG_PRODUCT) >> $(IMAGE)
endif
ifdef CONFIG_USER_NETFLASH_HMACMD5
	$(ROOTDIR)/user/netflash/hmacmd5 $(CONFIG_USER_NETFLASH_HMACMD5_KEY) < $(IMAGE) >> $(IMAGE)
else
	$(TOOLS)/cksum -b -o 2 $(IMAGE) >> $(IMAGE)
endif
endif

