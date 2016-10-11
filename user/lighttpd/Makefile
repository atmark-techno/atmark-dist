#
# Makefile for lighttpd
#
CONF_BUILD = $(shell ./config.guess)
CONF_HOST = $(shell ./config.sub $(CROSS:-=))

CONFIGURE_OPT =	--build=$(CONF_BUILD) --host=$(CONF_HOST) \
		--prefix=/usr --libdir=/usr/lib/lighttpd --with-openssl

all: build_targets

build/config.status:
	touch src/mod_auth.h
	mkdir -p build
	chmod +x configure
	(cd build; ../configure $(CONFIGURE_OPT);)

build_targets: build/config.status
	make -C build

	DESTDIR=`pwd`/install make -C build install

romfs:
	[ -d $(ROMFSDIR)/usr/sbin ] || mkdir -p $(ROMFSDIR)/usr/sbin
	(cd install/usr/sbin; \
	 $(ROMFSINST) /usr/sbin/lighttpd)
	[ -d $(ROMFSDIR)/usr/bin ] || mkdir -p $(ROMFSDIR)/usr/bin
	(cd install/usr/bin; \
	 $(ROMFSINST) /usr/bin/spawn-fcgi)
	[ -d $(ROMFSDIR)/usr/lib/lighttpd ] || mkdir $(ROMFSDIR)/usr/lib/lighttpd
	(cd install/usr/lib/lighttpd; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_access.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_accesslog.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_alias.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_auth.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_cgi.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_cml.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_compress.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_dirlisting.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_evasive.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_evhost.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_expire.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_fastcgi.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_flv_streaming.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_indexfile.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_magnet.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_mysql_vhost.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_proxy.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_redirect.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_rewrite.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_rrdtool.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_scgi.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_secdownload.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_setenv.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_simple_vhost.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_ssi.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_staticfile.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_status.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_trigger_b4_dl.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_userdir.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_usertrack.so; \
	 $(ROMFSINST) /usr/lib/lighttpd/mod_webdav.so)

clean:
	-make -C build clean

distclean: clean
	rm -rf build install
