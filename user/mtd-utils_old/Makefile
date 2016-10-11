
# -*- sh -*-

# $Id: Makefile,v 1.60 2005/11/07 11:15:09 gleixner Exp $

#SBINDIR=/usr/sbin
#MANDIR=/usr/man
#INCLUDEDIR=/usr/include
#CROSS=arm-linux-
#CC := $(CROSS)gcc
#CFLAGS := -I./include -O2 -Wall
CFLAGS += -I./include
BUILD_CFLAGS += -I./include
BUILD_CC = gcc

TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_ERASE)	+= flash_erase
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_ERASEALL)	+= flash_eraseall
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_INFO)	+= flash_info
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_LOCK)	+= flash_lock
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_UNLOCK)	+= flash_unlock
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_OTP_INFO)	+= flash_otp_info
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_OTP_DUMP)	+= flash_otp_dump
TARGETS_$(CONFIG_USER_MTDUTILS_FLASHCP)		+= flashcp
TARGETS_$(CONFIG_USER_MTDUTILS_FTL_CHECK)	+= ftl_check
TARGETS_$(CONFIG_USER_MTDUTILS_FTL_FORMAT)	+= ftl_format
TARGETS_$(CONFIG_USER_MTDUTILS_NFTL_FORMAT)	+= nftl_format
TARGETS_$(CONFIG_USER_MTDUTILS_NFTLDUMP)	+= nftldump
TARGETS_$(CONFIG_USER_MTDUTILS_RFDFORMAT)	+= rfdformat
TARGETS_$(CONFIG_USER_MTDUTILS_RFDDUMP)		+= rfddump
TARGETS_$(CONFIG_USER_MTDUTILS_NANDDUMP)	+= nanddump
TARGETS_$(CONFIG_USER_MTDUTILS_NANDWRITE)	+= nandwrite
TARGETS_$(CONFIG_USER_MTDUTILS_DOC_LOADBIOS)	+= doc_loadbios
TARGETS_$(CONFIG_USER_MTDUTILS_DOCFDISK)	+= docfdisk
TARGETS_$(CONFIG_USER_MTDUTILS_MKFSJFFS)	+= mkfs.jffs
TARGETS_$(CONFIG_USER_MTDUTILS_MKFSJFFS2)	+= mkfs.jffs2
TARGETS_$(CONFIG_USER_MTDUTILS_JFFS2DUMP)	+= jffs2dump
TARGETS_$(CONFIG_USER_MTDUTILS_MTD_DEBUG)	+= mtd_debug
TARGETS_$(CONFIG_USER_MTDUTILS_SUMTOOL)		+= sumtool

# Host based builds needed for constructing host images
TARGETS_$(CONFIG_JFFS_FS)			+= build/mkfs.jffs
TARGETS_$(CONFIG_JFFS2_FS)			+= build/mkfs.jffs2

SYMLINKS =
JFFS2_OBJS = crc32.o compr_rtime.o mkfs.jffs2.o compr_zlib.o compr.o
BUILD_JFFS2_OBJS = $(patsubst %,build/%,$(JFFS2_OBJS))

%: %.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/%: %.c
	$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $<

build/%.o: %.c
	$(BUILD_CC) $(BUILD_CFLAGS) -c -o $@ $<

.SUFFIXES:

all: build $(TARGETS_y)

#IGNORE=${wildcard .*.c.dep}
#-include ${IGNORE}

.PHONY:
build:
	[ -d build ] || mkdir build

clean:
	rm -f *.o \
	    $(TARGETS_y) $(TARGETS_n) $(TARGETS_m) $(TARGETS_) $(SYMLINKS)
	rm -rf build

romfs:
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FLASH_ERASE    /bin/flash_erase
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FLASH_ERASEALL /bin/flash_eraseall
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FLASH_INFO     /bin/flash_info
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FLASH_LOCK     /bin/flash_lock
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FLASH_UNLOCK   /bin/flash_unlock
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FLASH_OTP_INFO /bin/flash_otp_info
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FLASH_OTP_DUMP /bin/flash_otp_dump
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FLASHCP        /bin/flashcp
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FTL_CHECK      /bin/ftl_check
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FTL_FORMAT     /bin/ftl_format
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NFTL_FORMAT    /bin/nftl_format
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NFTLDUMP       /bin/nftldump
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_RFDFORMAT      /bin/rfdformat
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_RFDDUMP        /bin/rfddump
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDDUMP       /bin/nanddump
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDWRITE      /bin/nandwrite
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_DOC_LOADBIOS   /bin/doc_loadbios
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_DOCFDISK       /bin/docfdisk
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS       /bin/mkfs.jffs
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS2      /bin/mkfs.jffs2
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_JFFS2DUMP      /bin/jffs2dump
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MTD_DEBUG      /bin/mtd_debug
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_SUMTOOL        /bin/sumtool

$(SYMLINKS):
	ln -sf $(ROOTDIR)/$(LINUXDIR)/fs/jffs2/$@ $@

mkfs.jffs2: $(JFFS2_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lz $(LDLIBS)

flash_eraseall: crc32.o flash_eraseall.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

jffs2reader: jffs2reader.o
	$(CC) $(LDFLAGS) -o $@ $^ -lz $(LDLIBS)

jffs2dump: jffs2dump.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

sumtool: sumtool.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

build/mkfs.jffs2: $(BUILD_JFFS2_OBJS)
	$(BUILD_CC) -o $@ $^ -lz

#install: ${TARGETS}
#	mkdir -p ${DESTDIR}/${SBINDIR}
#	install -m0755 ${TARGETS} ${DESTDIR}/${SBINDIR}/
#	mkdir -p ${DESTDIR}/${MANDIR}/man1
#	gzip -c mkfs.jffs2.1 > ${DESTDIR}/${MANDIR}/man1/mkfs.jffs2.1.gz
