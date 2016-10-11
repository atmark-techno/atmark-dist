#
# Makefile for xorg-xserver
#
#

SRC_DIR_$(CONFIG_USER_XSERVER_1_4_2)  = xorg-xserver-1.4.2
SRC_DIR_$(CONFIG_USER_XSERVER_1_12_4)  = xorg-server-1.12.4
SRC_DIR = $(SRC_DIR_y)

CONF_BUILD = $(shell $(SRC_DIR)/config.guess)
CONF_HOST = $(shell $(SRC_DIR)/config.sub $(CROSS:-=))

CONFIGURE_OPT_1_12_4 =  --host=$(CONF_HOST) \
                        --build=$(CONF_BUILD) \
                        --prefix=/usr \
                        --with-fontrootdir=/usr/share/fonts/X11 \
                        --disable-docs \
                        --disable-devel-docs \
                        --disable-aiglx \
                        --disable-glx-tls \
                        --disable-registry \
                        --disable-composite \
                        --disable-mitshm \
                        --disable-xres \
                        --disable-record \
                        --disable-xv \
                        --disable-xvmc \
                        --disable-dga \
                        --disable-screensaver \
                        --disable-xdmcp \
                        --disable-xdm-auth-1 \
                        --disable-glx \
                        --disable-dri \
                        --disable-dri2 \
                        --disable-xinerama \
                        --disable-xf86vidmode \
                        --disable-xace \
                        --disable-dbe \
                        --disable-dpms \
                        --disable-dmx \
                        --disable-config-hal \
                        --disable-xfree86-utils \
                        --disable-xaa \
                        --disable-vgahw \
                        --disable-vbe \
                        --disable-int10-module \
                        --disable-libdrm \
                        --disable-clientids \
                        --disable-pciaccess \
                        --disable-xquartz \
                        --disable-xorg \
                        --disable-xvfb \
                        --disable-xnest \
                        --disable-xephyr \
                        --disable-secure-rpc \
                        --enable-tslib \
                        --enable-kdrive-kbd \
                        --enable-kdrive-mouse \
                        --enable-kdrive-evdev \
                        --enable-kdrive

CONFIGURE_OPT =	--host=$(CONF_HOST) \
                --build=$(CONF_BUILD) \
                --prefix=/usr \
                --with-fontdir=/usr/share/fonts/X11 \
                --disable-builddocs \
                --disable-build-linuxdoc \
                --disable-aiglx \
                --disable-registry \
                --disable-composite \
                --disable-shm \
                --disable-xres \
                --disable-xv \
                --disable-xvmc \
                --disable-dga \
                --disable-screensaver \
                --disable-xdmcp \
                --disable-xdm-auth-1 \
                --disable-glx \
                --disable-dri \
                --disable-dri2 \
                --disable-xinerama \
                --disable-xf86vidmode \
                --disable-xace \
                --disable-xevie \
                --disable-dbe \
                --disable-dpms \
                --disable-config-hal \
                --disable-xfree86-utils \
                --disable-xorg \
                --disable-xvfb \
                --disable-xnest \
                --disable-xephyr \
                --disable-xsdl \
                --disable-kdrive-vesa \
                --disable-secure-rpc \
                --enable-tslib \
                --enable-kdrive


ifeq "$(CONFIG_USER_XSERVER_1_12_4)" "y"
CONFIGURE_OPT = $(CONFIGURE_OPT_1_12_4)
endif

all: configure build

configure:
	(cd $(SRC_DIR); ./configure $(CONFIGURE_OPT) )

build: $(SRC_DIR)/Makefile
	make -C $(SRC_DIR) all

romfs: build
	 $(ROMFSINST) $(SRC_DIR)/hw/kdrive/fbdev/Xfbdev /usr/bin/Xfbdev

clean:
	make -C $(SRC_DIR) clean


distclean: clean
	make -C $(SRC_DIR) distclean

