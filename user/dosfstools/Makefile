#
# Makefile of dosfstools
#

SRC_DIR_y	= dosfstools-2.11
SRC_DIR		= $(SRC_DIR_y)

BIN_y =
BIN_$(CONFIG_USER_DOSFSTOOLS_DOSFSCK_DOSFSCK)	+= /sbin/dosfsck
BIN_$(CONFIG_USER_DOSFSTOOLS_DOSFSLABEL_DOSFSLABEL) += /sbin/dosfslabel
BIN_$(CONFIG_USER_DOSFSTOOLS_MKDOSFS_MKDOSFS)	+= /sbin/mkdosfs

PREINSTALLDIR = `pwd`/install/

all: build

build:
	make -C $(SRC_DIR)

romfs:
	mkdir -p $(PREINSTALLDIR)
	make -C $(SRC_DIR) DESTDIR=$(PREINSTALLDIR) PREFIX= install-bin

	mkdir -p $(ROMFSDIR)/sbin/
	for bin in $(BIN_y); do \
                $(ROMFSINST) $(PREINSTALLDIR)/$$bin $$bin; \
        done

	[ "$(CONFIG_USER_DOSFSTOOLS_DOSFSCK_DOSFSCK)" != "y" ] || \
	$(ROMFSINST) -s dosfsck /sbin/fsck.msdos && \
	$(ROMFSINST) -s dosfsck /sbin/fsck.vfat
	[ "$(CONFIG_USER_DOSFSTOOLS_MKDOSFS_MKDOSFS)" != "y" ] || \
	$(ROMFSINST) -s mkdosfs /sbin/mkfs.msdos && \
	$(ROMFSINST) -s mkdosfs /sbin/mkfs.vfat

clean distclean:
	rm -rf $(PREINSTALLDIR)
	make -C $(SRC_DIR) distclean
