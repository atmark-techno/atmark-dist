
WITHOUT_XATTR = 1

BUILD_DIR = build
INSTALL_DIR = preinstall

BUILDDIR = $(shell pwd)/$(BUILD_DIR)
DESTDIR = $(shell pwd)/$(INSTALL_DIR)

TARGETS_y = 
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_ERASE)	+= sbin/flash_erase
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_ERASEALL)	+= sbin/flash_eraseall
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_INFO)	+= sbin/flash_info
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_LOCK)	+= sbin/flash_lock
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_UNLOCK)	+= sbin/flash_unlock
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_OTP_INFO)	+= sbin/flash_otp_info
TARGETS_$(CONFIG_USER_MTDUTILS_FLASH_OTP_DUMP)	+= sbin/flash_otp_dump
TARGETS_$(CONFIG_USER_MTDUTILS_FLASHCP)		+= sbin/flashcp
TARGETS_$(CONFIG_USER_MTDUTILS_FTL_CHECK)	+= sbin/ftl_check
TARGETS_$(CONFIG_USER_MTDUTILS_FTL_FORMAT)	+= sbin/ftl_format
TARGETS_$(CONFIG_USER_MTDUTILS_NFTL_FORMAT)	+= sbin/nftl_format
TARGETS_$(CONFIG_USER_MTDUTILS_NFTLDUMP)	+= sbin/nftldump
TARGETS_$(CONFIG_USER_MTDUTILS_RFDFORMAT)	+= sbin/rfdformat
TARGETS_$(CONFIG_USER_MTDUTILS_RFDDUMP)		+= sbin/rfddump
TARGETS_$(CONFIG_USER_MTDUTILS_NANDDUMP)	+= sbin/nanddump
TARGETS_$(CONFIG_USER_MTDUTILS_NANDWRITE)	+= sbin/nandwrite
TARGETS_$(CONFIG_USER_MTDUTILS_NANDTEST)	+= sbin/nandtest
TARGETS_$(CONFIG_USER_MTDUTILS_DOC_LOADBIOS)	+= sbin/doc_loadbios
TARGETS_$(CONFIG_USER_MTDUTILS_DOCFDISK)	+= sbin/docfdisk
TARGETS_$(CONFIG_USER_MTDUTILS_MKFSJFFS2)	+= sbin/mkfs.jffs2
TARGETS_$(CONFIG_USER_MTDUTILS_JFFS2DUMP)	+= sbin/jffs2dump
TARGETS_$(CONFIG_USER_MTDUTILS_MTD_DEBUG)	+= sbin/mtd_debug
TARGETS_$(CONFIG_USER_MTDUTILS_SUMTOOL)		+= sbin/sumtool
TARGETS_$(CONFIG_USER_MTDUTILS_RECV_IMAGE)	+= sbin/recv_image
TARGETS_$(CONFIG_USER_MTDUTILS_SERVE_IMAGE)	+= sbin/serve_image

all:: all install

romfs:: all install
	for target in $(TARGETS_y); do \
		$(ROMFSINST) $(INSTALL_DIR)/usr/$$target /$$target;\
	done

distclean:: clean
	rm -rf $(DESTDIR)
	rm -f *~
