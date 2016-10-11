
SRC_DIR = udev-105

UDEV_DIR = $(shell pwd)
INSTALL_DIR = preinstall

TARGET_y = 
TARGET_$(CONFIG_USER_UDEV_UDEVD)		+= sbin/udevd
TARGET_$(CONFIG_USER_UDEV_UDEVTRIGGER)		+= sbin/udevtrigger
TARGET_$(CONFIG_USER_UDEV_UDEVSETTLE)		+= sbin/udevsettle
TARGET_$(CONFIG_USER_UDEV_UDEVCONTROL)		+= sbin/udevcontrol
TARGET_$(CONFIG_USER_UDEV_UDEVMONITOR)		+= usr/sbin/udevmonitor
TARGET_$(CONFIG_USER_UDEV_UDEVINFO)		+= usr/bin/udevinfo
EXTRA_y = 
EXTRA_$(CONFIG_USER_UDEV_EXTRA_ATA_ID)		+= extras/ata_id
EXTRA_$(CONFIG_USER_UDEV_EXTRA_CDROM_ID)	+= extras/cdrom_id
EXTRA_$(CONFIG_USER_UDEV_EXTRA_DASD_ID)		+= extras/dasd_id
EXTRA_$(CONFIG_USER_UDEV_EXTRA_EDD_ID)		+= extras/edd_id
EXTRA_$(CONFIG_USER_UDEV_EXTRA_PATH_ID)		+= extras/path_id
EXTRA_$(CONFIG_USER_UDEV_EXTRA_SCSI_ID)		+= extras/scsi_id
EXTRA_$(CONFIG_USER_UDEV_EXTRA_USB_ID)		+= extras/usb_id
EXTRA_$(CONFIG_USER_UDEV_EXTRA_VOLUME_ID)	+= extras/volume_id
EXTRA_$(CONFIG_USER_UDEV_EXTRA_FIRMWARE)	+= extras/firmware

all: build

build:
	make -C $(SRC_DIR) all
	mkdir -p $(INSTALL_DIR)
	make -C $(SRC_DIR) \
		DESTDIR=$(UDEV_DIR)/$(INSTALL_DIR) \
		EXTRAS="$(EXTRA_y)" \
		install

romfs: build
	for target in $(TARGET_y); do \
		$(ROMFSINST) $(INSTALL_DIR)/$$target /$$target;\
	done

	if [ "$(CONFIG_USER_UDEV_EXTRA_VOLUME_ID)" = "y" ]; then \
		$(ROMFSINST) $(INSTALL_DIR)/lib/libvolume_id.so.0.75.0 /lib/ ;\
		$(ROMFSINST) -s /lib/libvolume_id.so.0.75.0 \
				/lib/libvolume_id.so.0 ;\
	fi

	if [ -e $(INSTALL_DIR)/lib/udev ]; then \
		mkdir -p $(ROMFSDIR)/lib/udev ;\
		$(ROMFSINST) $(INSTALL_DIR)/lib/udev /lib/udev ;\
	fi

	if [ "$(CONFIG_USER_UDEV_EXTRA_FIRMWARE)" = "y" ]; then \
		mkdir -p $(ROMFSDIR)/etc/udev/rules.d ;\
		$(ROMFSINST) etc/udev/rules.d/z50-firmware.rules \
			     /etc/udev/rules.d ;\
	fi

clean:
	make -C $(SRC_DIR) \
		EXTRAS="$(EXTRA_y) $(EXTRA_)" \
		clean

distclean: clean
	rm -rf $(INSTALL_DIR)
