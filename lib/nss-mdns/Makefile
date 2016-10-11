CONF_BUILD = $(shell ./config.guess)
CONF_HOST = $(shell ./config.sub $(CROSS:-=))

all: build

build:
	rm -rf build install config.cache
	cp config.cache.cross config.cache
	mkdir build install
	chmod u+x configure
	( \
		cd build; \
                export CC='$(CC) $(CFLAGS)'; \
                export LDFLAGS='$(LDFLAGS)'; \
		../configure --prefix=`pwd`/../install --build=$(CONF_BUILD) --host=$(CONF_HOST) --cache-file=`pwd`/../config.cache; \
		make; \
		make install; \
	)

romfs:
	$(ROMFSINST) install/lib/libnss_mdns.so.2 /lib/libnss_mdns.so.2

clean:
	rm -rf build install
	rm config.cache
