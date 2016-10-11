#
# Makefile for avahi
#

CONF_BUILD = $(shell ./config.guess)
CONF_HOST = $(shell ./config.sub $(CROSS:-=))

CONFIGURE_OPT =	--disable-glib --disable-qt3 --disable-qt4 \
		--disable-gtk --disable-gdbm --disable-python \
		--disable-pygtk --disable-python-dbus --disable-mono \
		--disable-monodoc --disable-doxygen-doc --disable-doxygen-dot \
		--disable-doxygen-html --disable-manpages --disable-xmltoman \
		--with-distro=none --prefix=`pwd`/../install \
		--build=$(CONF_BUILD) --host=$(CONF_HOST) \
		--cache-file=`pwd`/../config.cache --disable-manpages \
		--disable-dbus \
		CC="$(CC)" \
		CFLAGS="$(CFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		LIBS='$(LIBPTHREAD) $(LDLIBS)' \
		LIBDAEMON_CFLAGS="-D_REENTRANT $(CFLAGS)" \
		LIBDAEMON_LIBS="$(LDFLAGS) -ldaemon" \
		PKG_CONFIG=true

all: build_targets

build/config.status:
	mkdir -p build
	cp config.cache.cross config.cache

	chmod u+x configure
	touch Makefile.in aclocal.m4 config.h.in configure
	(cd build; ../configure $(CONFIGURE_OPT);)

build_targets: build/config.status
	make -C build prefix="" exec_prefix="" all

	mkdir -p install
	make -C build install

romfs:
	[ -d $(ROMFSDIR)/sbin ] || mkdir -p $(ROMFSDIR)/sbin
	$(ROMFSINST) install/sbin/avahi-daemon /sbin/avahi-daemon
	[ -d $(ROMFSDIR)/lib ] || mkdir $(ROMFSDIR)/lib
	$(ROMFSINST) install/lib/libavahi-common.so.3.3.0 /lib/libavahi-common.so.3.3.0
	$(ROMFSINST) -s /lib/libavahi-common.so.3.3.0 /lib/libavahi-common.so
	$(ROMFSINST) -s /lib/libavahi-common.so.3.3.0 /lib/libavahi-common.so.3
	$(ROMFSINST) install/lib/libavahi-core.so.4.0.1 /lib/libavahi-core.so.4.0.1
	$(ROMFSINST) -s /lib/libavahi-core.so.4.0.1 /lib/libavahi-core.so
	$(ROMFSINST) -s /lib/libavahi-core.so.4.0.1 /lib/libavahi-core.so.4

clean:
	-make -C build clean

distclean: clean
	rm -rf build install
	rm -f config.cache config.log
