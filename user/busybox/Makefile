#
# Makefile of busybox
#

SRC_DIR_$(CONFIG_USER_BUSYBOX_1_00_RC3)	= busybox-1.00.rc3
SRC_DIR_$(CONFIG_USER_BUSYBOX_1_20_2)	= busybox-1.20.2
SRC_DIR = $(SRC_DIR_y)

all: build

build:
	make -C $(SRC_DIR)
	ln -sf $(SRC_DIR)/busybox

romfs-busybox-1.00.rc3:
	make -C $(SRC_DIR) romfs
romfs-busybox-1.20.2:
	make -C $(SRC_DIR) CONFIG_PREFIX=$(ROMFSDIR) install
romfs: romfs-$(SRC_DIR)

clean:
	rm -f busybox
	make -C $(SRC_DIR) clean

distclean:
	rm -f busybox
	make -C $(SRC_DIR) distclean
