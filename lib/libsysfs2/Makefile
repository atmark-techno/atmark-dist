
SRC_DIR = sysfsutils-2.1.0

UDEV_DIR = $(shell pwd)
INSTALL_DIR = preinstall

CONF_BUILD = $(shell $(SRC_DIR)/config.guess)
CONF_HOST = $(shell $(SRC_DIR)/config.sub $(CROSS:-=))
CONF_OPT = 
CONF_OPT_MISC = 

LIB_y =
LIB_$(CONFIG_LIB_LIBSYSFS2)		+= usr/local/lib/libsysfs.so.2.0.1 
LIB_$(CONFIG_LIB_LIBSYSFS2_FORCE)	+= usr/local/lib/libsysfs.so.2.0.1 
BIN_y =
BIN_$(CONFIG_LIB_LIBSYSFS2_SYSTOOL)	+= usr/local/bin/systool
BIN_$(CONFIG_LIB_LIBSYSFS2_GET_DEVICE)	+= usr/local/bin/get_device
BIN_$(CONFIG_LIB_LIBSYSFS2_GET_DRIVER)	+= usr/local/bin/get_driver
BIN_$(CONFIG_LIB_LIBSYSFS2_GET_MODULE)	+= usr/local/bin/get_module

GRANT_EXE = config.guess config.sub configure missing depcomp 
GRANT_EXE += test/create-test

all: build

$(SRC_DIR)/Makefile:
	for file in $(GRANT_EXE); do \
		chmod +x $(SRC_DIR)/$$file;\
	done

	(cd $(SRC_DIR); \
	 ./configure --build=$(CONF_BUILD) --host=$(CONF_HOST) \
	             $(CONF_OPT) $(CONF_OPT_MISC));\

build: $(SRC_DIR)/Makefile
	make -C $(SRC_DIR) all
	mkdir -p $(INSTALL_DIR)
	make -C $(SRC_DIR) DESTDIR=$(UDEV_DIR)/$(INSTALL_DIR) install

romfs: build
	for target in $(LIB_y); do \
		$(ROMFSINST) $(INSTALL_DIR)/$$target \
			/lib/`basename $$target`;\
	done
	for target in $(BIN_y); do \
		[ -e $(ROMFSDIR)/usr/bin ] || mkdir -p $(ROMFSDIR)/lib/bin; \
		$(ROMFSINST) $(INSTALL_DIR)/$$target \
			/usr/bin/`basename $$target`;\
	done

	[ "$(CONFIG_LIB_LIBSYSFS2)" != "y" ] && \
	[ "$(CONFIG_LIB_LIBSYSFS2_FORCE)" != "y" ] || \
	(cd $(ROMFSDIR)/lib; \
	for link in libsysfs.so libsysfs.so.2; do \
		ln -fs /lib/libsysfs.so.2.0.1 $$link;\
	done)

clean:
	if [ -f $(SRC_DIR)/Makefile ]; then \
		make -C $(SRC_DIR) clean ;\
	fi

distclean: clean
	if [ -f $(SRC_DIR)/Makefile ]; then \
		make -C $(SRC_DIR) distclean ;\
	fi
	find $(SRC_DIR) -name \*~ -exec rm -f {} \;
	rm -rf $(INSTALL_DIR)
