#
# Makefile for gdbserver
#

BIN_PERM = 0755

ifeq ($(filter %-linux-,$(CROSS_COMPILE)),)
	TRIPLET = $(CROSS_COMPILE:-=)
	CROSS_GDBDIR = /usr/share/gdb-$(TRIPLET)
else
	TRIPLET = $(CROSS_COMPILE)gnu
	CROSS_GDBDIR = /usr/share/gdb-$(TRIPLET)/bin
endif

TARGET_y =
TARGET_$(CONFIG_USER_GDBSERVER_GDBREPLAY) += $(CROSS_GDBDIR)/gdbreplay
TARGET_$(CONFIG_USER_GDBSERVER_GDBSERVER) += $(CROSS_GDBDIR)/gdbserver

all: $(TARGET_y)

romfs: $(TARGET_y)
	$(foreach target,$^,$(ROMFSINST) -p $(BIN_PERM) $(target) \
	/bin/$(notdir $(target));)

$(TARGET_y):
	$(error No $@ found, please install the latest version of gdb-$(TRIPLET) \
	package from download.atmark-techno.com)

clean:
distclean:
