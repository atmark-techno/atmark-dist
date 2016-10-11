
SRC_DIR = udev-175

BUILD_DIR = builddir

CONF_BUILD = $(shell $(SRC_DIR)/build-aux/config.guess)
CONF_HOST = $(shell $(SRC_DIR)/build-aux/config.sub $(CROSS:-=))

CONFIGURE_OPT = --build=$(CONF_BUILD)				\
		--host=$(CONF_HOST)				\
		--sysconfdir=/etc --sbindir=/sbin		\
		--libexecdir=/lib/udev				\
		--enable-static					\
		--disable-dependency-tracking	\
		--disable-gtk-doc --disable-introspection	\
		--disable-edd --disable-gudev --disable-keymap	\
		--with-systemdsystemunitdir=no			\
		--with-usb-ids-path=../usb.ids			\
		--with-pci-ids-path=../pci.ids
ifeq ($(CONFIG_USER_UDEV_EXTRAS_MTD_PROBE),y)
CONFIGURE_OPT += --enable-mtd_probe
else
CONFIGURE_OPT += --disable-mtd_probe
endif
ifeq ($(CONFIG_USER_UDEV_EXTRAS_RULE_GENERATOR),y)
CONFIGURE_OPT += --enable-rule_generator
else
CONFIGURE_OPT += --disable-rule_generator
endif
ifeq ($(CONFIG_USER_UDEV_EXTRAS_UDEV_ACL),y)
CONFIGURE_OPT += --enable-udev_acl
else
CONFIGURE_OPT += --disable-udev_acl
endif

ROMFS_DIRS_$(CONFIG_USER_UDEV) = \
	sbin \
	etc/default \
	etc/init.d \
	etc/rc.d \
	etc/udev \
	lib/udev/rules.d
ROMFS_DIRS_$(CONFIG_USER_UDEV_EXTRAS_UDEV_ACL)	+= \
	usr/lib/ConsoleKit/run-seat.d

SBIN_$(CONFIG_USER_UDEV)			= \
	$(BUILD_DIR)/udev/udevd \
	$(BUILD_DIR)/udev/udevadm

CONF_$(CONFIG_USER_UDEV_DEFAULT_SCRIPT_CONF)			= \
	$(SRC_DIR)/udev.conf \
	$(SRC_DIR)/extra/links.conf

LIB_$(CONFIG_USER_UDEV)				= \
	$(SRC_DIR)/extra/create_static_nodes \
	$(SRC_DIR)/extra/firmware.agent \
	$(SRC_DIR)/extra/hotplug.functions \
	$(SRC_DIR)/extra/logger.agent \
	$(SRC_DIR)/extra/net.agent \
	$(SRC_DIR)/extra/write_dev_root_rule
LIB_$(CONFIG_USER_UDEV_EXTRAS_ACCELEROMETER)	+= \
	$(BUILD_DIR)/extras/accelerometer/accelerometer
LIB_$(CONFIG_USER_UDEV_EXTRAS_ATA_ID)		+= \
	$(BUILD_DIR)/extras/ata_id/ata_id
LIB_$(CONFIG_USER_UDEV_EXTRAS_CDROM_ID)		+= \
	$(BUILD_DIR)/extras/cdrom_id/cdrom_id
LIB_$(CONFIG_USER_UDEV_EXTRAS_COLLECT)		+= \
	$(BUILD_DIR)/extras/collect/collect
LIB_$(CONFIG_USER_UDEV_EXTRAS_MTD_PROBE)	+= \
	$(BUILD_DIR)/extras/mtd_probe/mtd_probe
LIB_$(CONFIG_USER_UDEV_EXTRAS_SCSI_ID)		+= \
	$(BUILD_DIR)/extras/scsi_id/scsi_id
LIB_$(CONFIG_USER_UDEV_EXTRAS_V4L_ID)		+= \
	$(BUILD_DIR)/extras/v4l_id/v4l_id
LIB_$(CONFIG_USER_UDEV_EXTRAS_UDEV_ACL)		+= \
	$(BUILD_DIR)/extras/udev-acl/udev-acl
LIB_$(CONFIG_USER_UDEV_EXTRAS_USB_DB)		+= \
	$(BUILD_DIR)/extras/usb-db/pci-db \
	$(BUILD_DIR)/extras/usb-db/usb-db
LIB_$(CONFIG_USER_UDEV_EXTRAS_RULE_GENERATOR)	+= \
	$(SRC_DIR)/extras/rule_generator/rule_generator.functions \
	$(SRC_DIR)/extras/rule_generator/write_cd_rules \
	$(SRC_DIR)/extras/rule_generator/write_net_rules

RULES_$(CONFIG_USER_UDEV_DEFAULT_RULES)			= \
	$(SRC_DIR)/rules/debian/50-udev-default.rules \
	$(SRC_DIR)/rules/debian/60-persistent-storage-tape.rules \
	$(SRC_DIR)/rules/debian/60-persistent-storage.rules \
	$(SRC_DIR)/rules/rules.d/60-persistent-serial.rules \
	$(SRC_DIR)/rules/rules.d/60-persistent-input.rules \
	$(SRC_DIR)/rules/rules.d/60-persistent-alsa.rules \
	$(SRC_DIR)/rules/debian/80-drivers.rules \
	$(SRC_DIR)/rules/debian/80-networking.rules \
	$(SRC_DIR)/rules/debian/91-permissions.rules \
	$(SRC_DIR)/rules/rules.d/95-udev-late.rules
RULES_$(CONFIG_USER_UDEV_EXTRAS_ACCELEROMETER)	+= \
	$(SRC_DIR)/extras/accelerometer/61-accelerometer.rules
RULES_$(CONFIG_USER_UDEV_EXTRAS_CDROM_ID)	+= \
	$(SRC_DIR)/extras/cdrom_id/60-cdrom_id.rules
RULES_$(CONFIG_USER_UDEV_EXTRAS_FIRMWARE)	+= \
	$(SRC_DIR)/extras/firmware/50-firmware.rules
RULES_$(CONFIG_USER_UDEV_EXTRAS_MTD_PROBE)	+= \
	$(SRC_DIR)/extras/mtd_probe/75-probe_mtd.rules
RULES_$(CONFIG_USER_UDEV_EXTRAS_RULE_GENERATOR)	+= \
	$(SRC_DIR)/rules/debian/75-cd-aliases-generator.rules \
	$(SRC_DIR)/rules/debian/75-persistent-net-generator.rules
RULES_$(CONFIG_USER_UDEV_EXTRAS_UDEV_ACL)	+= \
	$(SRC_DIR)/extras/udev-acl/70-udev-acl.rules
RULES_$(CONFIG_USER_UDEV_EXTRAS_USB_DB)		+= \
	$(SRC_DIR)/rules/rules.d/75-net-description.rules \
	$(SRC_DIR)/rules/rules.d/75-tty-description.rules \
	$(SRC_DIR)/rules/rules.d/78-sound-card.rules
RULES_$(CONFIG_USER_UDEV_EXTRAS_V4L_ID)		+= \
	$(SRC_DIR)/extras/v4l_id/60-persistent-v4l.rules

TARGET_$(CONFIG_USER_UDEV_DEFAULT_SCRIPT_CONF)			= etc

all: build

$(BUILD_DIR)/Makefile: $(CONFIG_CONFIG)
	touch $(SRC_DIR)/Makefile.in
	touch $(SRC_DIR)/extras/gudev/docs/Makefile.in
	touch $(SRC_DIR)/libudev/docs/Makefile.in
	rm -rf $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)
	(cd $(BUILD_DIR); ../$(SRC_DIR)/configure $(CONFIGURE_OPT))

build: $(BUILD_DIR)/Makefile
	make -C $(BUILD_DIR) all

romfs: build
	@for dir in $(ROMFS_DIRS_y); do \
		[ -d $(ROMFSDIR)/$$dir ] || mkdir -p $(ROMFSDIR)/$$dir; \
	done
	@for src in $(SBIN_y); do \
		dst=$$(printf "/sbin/%s" $$(basename $$src)); \
		$(ROMFSINST) $$src $$dst; \
	done
	$(ROMFSINST) $(BUILD_DIR)/libudev/.libs/libudev.so.0.13.0 \
		/lib/libudev.so.0.13.0
	$(ROMFSINST) -s libudev.so.0.13.0 /lib/libudev.so.0
	@for src in $(CONF_y); do \
		if [ "$(CONFIG_USER_FLATFSD_FLATFSD)" = "y" ]; then \
			target=$$(printf "/etc/config/%s" $$(basename $$src)); \
			link=$$(printf "/etc/udev/%s" $$(basename $$src)); \
			$(ROMFSINST) -s $$target $$link; \
			dst=$$(printf "/etc/default/%s" $$(basename $$src)); \
		else \
			dst=$$(printf "/etc/udev/%s" $$(basename $$src)); \
		fi; \
		$(ROMFSINST) $$src $$dst; \
	done
	@for src in $(LIB_y); do \
		dst=$$(printf "/lib/udev/%s" $$(basename $$src)); \
		$(ROMFSINST) $$src $$dst; \
		chmod 755 $(ROMFSDIR)$$dst; \
	done
	@for src in $(RULES_y); do \
		dst=$$(printf "/lib/udev/rules.d/%s" $$(basename $$src)); \
		if [ $$src = "$(SRC_DIR)/rules/debian/60-persistent-storage.rules" ]; then \
			cat $$src | sed \
				-e 's/\(KERNEL==\"sd\*|hd\*|cciss\*\", \\\)$$/#\1/' \
				-e 's/\(\tIMPORT{program}=\"edd_id --export \$$tempnode\"\)/#\1/' \
				> $(ROMFSDIR)/$$dst; \
		else \
			$(ROMFSINST) $$src $$dst; \
		fi; \
	done
	@if [ "$(CONFIG_USER_UDEV_EXTRAS_UDEV_ACL)" = "y" ]; then \
		$(ROMFSINST) -s /lib/udev/udev-acl \
			/usr/lib/ConsoleKit/run-seat.d/udev-acl.ck; \
	fi;
	@for src in $(TARGET_y); do \
		dst=$$(printf "/%s" $$(basename $$src)); \
		$(ROMFSINST) $$dst; \
	done

clean:
	rm -rf $(BUILD_DIR)

distclean: clean
