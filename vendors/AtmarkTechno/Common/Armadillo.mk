#
# Armadillo.mk -- Armadillo-series common settings
#

LINUXBIN  = $(IMAGEDIR)/linux.bin
ZLINUXBIN = $(IMAGEDIR)/linux.bin.gz
ROMFSIMG  = $(IMAGEDIR)/romfs.img
ZROMFSIMG = $(IMAGEDIR)/romfs.img.gz
CKSUM     = $(ROOTDIR)/tools/cksum
IMAGE     = $(ROMFSIMG)

TOOLS_DIR = $(ROOTDIR)/vendors/$(CONFIG_VENDOR)/Common/tools
TERMINFO  = $(ROOTDIR)/user/terminfo/termtypes.ti

ROMFS_DIRS = \
	bin \
	boot \
	dev \
	dev/flash \
	dev/input \
	etc \
	etc/avahi \
	etc/config \
	etc/default \
	etc/dhcpc \
	etc/network \
	etc/network/if-down.d \
	etc/network/if-post-down.d \
	etc/network/if-pre-up.d \
	etc/network/if-up.d \
	etc/rc.d \
	etc/terminfo \
	home \
	home/ftp \
	home/ftp/bin \
	home/ftp/etc \
	home/ftp/lib \
	home/ftp/pub \
	home/guest \
	home/www-data \
	home/www-data/admin \
	home/www-data/cgi-bin \
	lib \
	media \
	mnt \
	opt \
	opt/firmware \
	opt/license \
	proc \
	root \
	run \
	sbin \
	sys \
	tmp \
	usr \
	usr/bin \
	usr/sbin \
	usr/lib \
	usr/share \
	var \
	var/cache \
	var/lib \
	var/lock \
	var/log \
	var/run \
	var/spool \
	var/tmp

TERMS = \
	Eterm \
	Eterm-color \
	ansi \
	cons25 \
	cygwin \
	dumb \
	hurd \
	linux \
	mach \
	mach-bold \
	mach-color \
	pcansi \
	rxvt \
	rxvt-basic \
	rxvt-m \
	screen \
	screen-bce \
	screen-s \
	screen-w \
	sun \
	vt100 \
	vt102 \
	vt220 \
	vt52 \
	wsvt25 \
	wsvt25m \
	xterm \
	xterm-color \
	xterm-debian \
	xterm-mono \
	xterm-r5 \
	xterm-r6 \
	xterm-vt220 \
	xterm-xfree86
