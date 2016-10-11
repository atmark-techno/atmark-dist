#
# Makefile for xkb-data
#
#

SRC_DIR = xkeyboard-config-2.5.1
INSTALL_DIR = .INSTALL
PWD = $(shell pwd)

CONF_BUILD = $(shell $(SRC_DIR)/config.guess)
CONF_HOST = $(shell $(SRC_DIR)/config.sub $(CROSS:-=))

CONFIGURE_OPT =	--host=$(CONF_HOST) \
                --build=$(CONF_BUILD) \
                --prefix=$(PWD)/$(INSTALL_DIR)

all: configure build

configure:
	(cd $(SRC_DIR); ./configure $(CONFIGURE_OPT) )

build: $(SRC_DIR)/Makefile
	make -C $(SRC_DIR) all install

romfs: build
	$(ROMFSINST) $(INSTALL_DIR)/share/X11/xkb /usr/share/X11/xkb

clean:
	make -C $(SRC_DIR) clean
	-rm -rf $(INSTALL_DIR)

distclean: clean
	make -C $(SRC_DIR) distclean
