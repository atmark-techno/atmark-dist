#
# Makefile of squashfs-tools
#

SRC_DIR = squashfs-tools-4.2/squashfs-tools

BIN_y =
BIN_$(CONFIG_USER_SQUASHFSTOOLS_MKSQUASHFS)	+= /usr/bin/mksquashfs
BIN_$(CONFIG_USER_SQUASHFSTOOLS_UNSQUASHFS)	+= /usr/bin/unsquashfs

BUILD_OPTIONS =
BUILD_OPTIONS += ${shell pkg-config --exists liblzma && echo XZ_SUPPORT=1}

all: build

build:
	make -C $(SRC_DIR) CC=$(CC) $(BUILD_OPTIONS)

romfs:
	mkdir -p $(ROMFSDIR)/usr/
	mkdir -p $(ROMFSDIR)/usr/bin

	for bin in $(BIN_y); do \
		$(ROMFSINST) $(SRC_DIR)/`basename $$bin` $$bin; \
	done

clean:
	make -C $(SRC_DIR) clean

distclean: clean
