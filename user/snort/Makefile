#
# Makefile for snort
#
CONF_BUILD = $(shell ./config.guess)
CONF_HOST = $(shell ./config.sub $(CROSS:-=))

CONFIGURE_OPT = CC="$(CC)" \
	CFLAGS="$(CFLAGS)" \
	LDFLAGS="$(LDFLAGS)" \
	LIBS="$(LIBPTHREAD) $(LDLIBS)" \
	--build=$(CONF_BUILD) \
	--host=$(CONF_HOST)

ifdef CONFIG_USER_SNORT_DYNAMICPLUGIN
CONFIGURE_OPT += --enable-dynamicplugin
endif

all: build_targets

build/config.status:
	[ -d build ] || mkdir build
	chmod u+x configure
	(cd build; ../configure $(CONFIGURE_OPT))

build_targets: build/config.status
	make -C build prefix="" exec_prefix="" all
	[ -d install ] || mkdir install
	make -C build install prefix=`pwd`/install

romfs:
	$(ROMFSINST) install/bin /bin
ifdef CONFIG_USER_SNORT_DYNAMICPLUGIN
	[ -d $(ROMFSDIR)/usr/lib ] || mkdir -p $(ROMFSDIR)/usr/lib
	$(ROMFSINST) install/lib /usr/lib 
endif

clean:
	-make -C build clean

distclean: clean
	rm -rf build install
	rm -f config.cache config.log

