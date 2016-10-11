SRC_DIR = joe-3.7

CONF_BUILD = $(shell $(SRC_DIR)/config.guess)
CONF_HOST = $(shell $(SRC_DIR)/config.sub $(CROSS:-=))

BUILD_DIR = builddir

CONFIGURE_OPT = --sysconfdir=/etc/config	\
	--build=$(CONF_BUILD)					\
	--host=$(CONF_HOST)

all: build

$(BUILD_DIR)/Makefile:
	mkdir -p $(BUILD_DIR)
	(cd $(BUILD_DIR); ../$(SRC_DIR)/configure $(CONFIGURE_OPT))

build: $(BUILD_DIR)/Makefile
	make -C $(BUILD_DIR)

romfs: build
	$(ROMFSINST) $(BUILD_DIR)/joe /usr/bin/

clean:
	make -C $(BUILD_DIR) clean

distclean: clean
	rm -rf $(BUILD_DIR)
