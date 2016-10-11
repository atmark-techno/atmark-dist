SRC_DIR = curl-7.26.0

CONF_BUILD = $(shell $(SRC_DIR)/config.guess)
CONF_HOST = $(shell $(SRC_DIR)/config.sub $(CROSS:-=))

BUILD_DIR = builddir

CONFIGURE_OPT = --build=$(CONF_BUILD)		\
		--host=$(CONF_HOST)		\
		--with-random=/dev/urandom	\
		--prefix=/usr			\
		--disable-dependency-tracking	\
		--enable-ipv6			\
		--with-lber-lib=lber		\
		--disable-symbol-hiding
ifeq ($(CONFIG_USER_CURL_WITH_IDN),y)
CONFIGURE_OPT += --with-libidn
else
CONFIGURE_OPT += --without-libidn
endif
ifeq ($(CONFIG_USER_CURL_WITH_LDAP),y)
CONFIGURE_OPT += --enable-ldap --enable-ldaps
else
CONFIGURE_OPT += --disable-ldap --disable-ldaps
endif
ifeq ($(CONFIG_USER_CURL_WITH_RTMP),y)
CONFIGURE_OPT += --with-librtmp
else
CONFIGURE_OPT += --without-librtmp
endif
ifeq ($(CONFIG_USER_CURL_WITH_SSH2),y)
CONFIGURE_OPT += --with-libssh2
else
CONFIGURE_OPT += --without-libssh2
endif

all: build

$(BUILD_DIR)/Makefile: $(CONFIG_CONFIG)
	mkdir -p $(BUILD_DIR)
	(cd $(BUILD_DIR); ../$(SRC_DIR)/configure $(CONFIGURE_OPT))

build: $(BUILD_DIR)/Makefile
	make -C $(BUILD_DIR)

romfs: build
	$(ROMFSINST) $(BUILD_DIR)/src/.libs/curl /usr/bin/curl
	$(ROMFSINST) $(BUILD_DIR)/lib/.libs/libcurl.so.4.2.0 \
		/lib/libcurl.so.4.2.0
	$(ROMFSINST) -s libcurl.so.4.2.0 /lib/libcurl.so.4
	$(ROMFSINST) -s libcurl.so.4.2.0 /lib/libcurl.so

clean:
	[ -d $(BUILD_DIR) ] && make -C $(BUILD_DIR) clean

distclean:
	rm -rf $(BUILD_DIR)
