#
# build using configure as best we can, should work for most targets
# David McCullough <davidm@snapgear.com>
#

all: build/config.cache
	$(MAKE) -C build MAKE="$(MAKE) -f Makefile"

romfs: build/config.cache
	$(MAKE) -C build MAKE="$(MAKE) -f Makefile" $@

build/config.cache: makefile
	rm -rf build; \
	export CFLAGS="$(VENDOR_CFLAGS) -DEMBED -O2"; \
	unset LDFLAGS; \
	chmod u+x configure; \
	mkdir build; \
	cd build; \
	../configure  --prefix= \
		$(GLIBC_TARGET_OPTS) --build=i386-redhat-linux \
		--with-headers=$(ROOTDIR)/$(LINUXDIR)/include \
		--datadir=/usr/share \
		--without-cvs --without-gd \
		--disable-profile --disable-force-install \
		--enable-add-ons

clean:
	rm -rf build

