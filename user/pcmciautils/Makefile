
SRC_DIR = pcmciautils-014

UDEV_DIR = $(shell pwd)
INSTALL_DIR = preinstall

TARGET_y = 
TARGET_$(CONFIG_USER_PCMCIAUTILS_PCCARDCTL)        += sbin/pccardctl
TARGET_$(CONFIG_USER_PCMCIAUTILS_CHECK_BROKEN_CIS) += sbin/pcmcia-check-broken-cis
TARGET_$(CONFIG_USER_PCMCIAUTILS_SOCKET_STARTUP)   += sbin/pcmcia-socket-startup

EXTRA_CFLGAS = -O2

ifeq ($(CONFIG_LIB_LIBSYSFS2_FORCE),y)
EXTRA_CFLAGS += -I$(ROOTDIR)/lib/libsysfs2/preinstall/usr/local/include
EXTRA_LDFLAGS += -L$(ROOTDIR)/lib/libsysfs2/preinstall/usr/local/lib
endif

all: build

build:
	make -C $(SRC_DIR) all \
		OPTIMIZATION="$(CFLAGS) $(EXTRA_CFLAGS)" \
		LDFLAGS="$(LDFLAGS) $(EXTRA_LDFLAGS)" \
		CC=$(CC) LD=$(CC) STRIP=$(STRIP)
	mkdir -p $(INSTALL_DIR)
	make -C $(SRC_DIR) DESTDIR=$(UDEV_DIR)/$(INSTALL_DIR) install

romfs: build
	for target in $(TARGET_y); do \
		$(ROMFSINST) $(INSTALL_DIR)/$$target /$$target;\
	done

clean:
	make -C $(SRC_DIR) clean

distclean: clean
	rm -rf $(INSTALL_DIR)
