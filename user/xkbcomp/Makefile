#
# Makefile for xkbcomp
#
#

SRC_DIR = xkbcomp-1.2.4

CONF_BUILD = $(shell $(SRC_DIR)/config.guess)
CONF_HOST = $(shell $(SRC_DIR)/config.sub $(CROSS:-=))

CONFIGURE_OPT =	--host=$(CONF_HOST) \
                --build=$(CONF_BUILD) \
                --prefix=/usr

all: configure build

configure:
	(cd $(SRC_DIR); ./configure $(CONFIGURE_OPT) )

build: $(SRC_DIR)/Makefile
	make -C $(SRC_DIR) all

romfs: build
	$(ROMFSINST) $(SRC_DIR)/xkbcomp /usr/bin/xkbcomp

clean:
	make -C $(SRC_DIR) clean

distclean: clean
	make -C $(SRC_DIR) distclean
