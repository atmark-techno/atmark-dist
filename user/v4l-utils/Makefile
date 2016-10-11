#
# Makefile of v4l-utils-0.8.8
#

SRC_DIR = v4l-utils-0.8.8
V4LUTILS_DIR = $(shell pwd)
INSTALL_DIR = preinstall

DIST_BUILD=y
DISABLE_QT=y

TOOLS_y=
TOOLS_$(CONFIG_USER_V4LUTILS_V4L2COMPLIANCE)+= v4l2-compliance
TOOLS_$(CONFIG_USER_V4LUTILS_V4L2CTL)       += v4l2-ctl
TOOLS_$(CONFIG_USER_V4LUTILS_V4L2DBG)       += v4l2-dbg

LIBS_y=
LIBS_$(CONFIG_USER_V4LUTILS_V4L2COMPLIANCE)+= libv4l2.so.0 libv4lconvert.so.0 libv4l/ov511-decomp libv4l/ov518-decomp
LIBS_$(CONFIG_USER_V4LUTILS_V4L2CTL)       += libv4l2.so.0 libv4lconvert.so.0 libv4l/ov511-decomp libv4l/ov518-decomp

export DIST_BUILD
export DISABLE_QT
export TOOLS_y

all: build

build:
	make -C $(SRC_DIR) \
		PREFIX= \
		all
	mkdir -p $(INSTALL_DIR)
	make -C $(SRC_DIR) \
		PREFIX= \
		DESTDIR=$(V4LUTILS_DIR)/$(INSTALL_DIR) \
		install

romfs: build
	mkdir -p $(ROMFSDIR)/bin
	mkdir -p $(ROMFSDIR)/sbin
	mkdir -p $(ROMFSDIR)/lib/libv4l
	$(ROMFSINST) $(INSTALL_DIR)/bin /bin
	$(ROMFSINST) $(INSTALL_DIR)/sbin /sbin
	for lib in $(LIBS_y); do \
		$(ROMFSINST) $(INSTALL_DIR)/lib/$$lib /lib/$$lib; \
	done

clean:
	make -C $(SRC_DIR) clean

distclean: clean
	rm -r $(INSTALL_DIR)
