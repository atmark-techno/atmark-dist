#
# Makefile of i2c-tools
#

SRC_DIR = i2c-tools-3.0.2

BIN_y =
BIN_$(CONFIG_USER_I2CTOOLS_I2CDETECT)		+= /usr/sbin/i2cdetect
BIN_$(CONFIG_USER_I2CTOOLS_I2CDUMP)		+= /usr/sbin/i2cdump
BIN_$(CONFIG_USER_I2CTOOLS_I2CGET)		+= /usr/sbin/i2cget
BIN_$(CONFIG_USER_I2CTOOLS_I2CSET)		+= /usr/sbin/i2cset

all: build

build:
	make -C $(SRC_DIR) CC=$(CC)

romfs: build
	mkdir -p $(ROMFSDIR)/usr/
	mkdir -p $(ROMFSDIR)/usr/sbin

	for bin in $(BIN_y); do \
		$(ROMFSINST) $(SRC_DIR)/tools/`basename $$bin` $$bin; \
	done

clean:
	make -C $(SRC_DIR) clean

distclean: clean
