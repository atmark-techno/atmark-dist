#
# Makefile for libarms
#

SRC_DIR = armsd-0.9.0

all:
	make -C $(SRC_DIR)

romfs:
	$(ROMFSINST) $(SRC_DIR)/armsd /usr/sbin/armsd

clean:
	-make -C $(BUILD_DIR) clean

distclean: clean
