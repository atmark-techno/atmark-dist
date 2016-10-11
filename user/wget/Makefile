#
# build using configure as best we can, should work for most targets
# David McCullough <davidm@snapgear.com>
#
CONF_BUILD = $(shell ./config.guess)
CONF_HOST = $(shell ./config.sub $(CROSS:-=))

all: build
	$(MAKE) -C build

build: Makefile
	rm -rf build
	mkdir build
	chmod u+x configure
	( \
		cd build; \
		export CC="$(CC) $(CFLAGS)"; \
		export LDFLAGS="$(LDFLAGS)"; \
		export LIBS="$(LDLIBS)"; \
		../configure --build=$(CONF_BUILD) --host=$(CONF_HOST) --prefix= ; \
	)

romfs:
	$(ROMFSINST) build/src/wget /bin/wget

clean:
	rm -rf build

