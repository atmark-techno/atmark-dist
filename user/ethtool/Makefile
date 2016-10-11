CONF_BUILD = $(shell ./config.guess)
CONF_HOST = $(shell ./config.sub $(CROSS:-=))

all: build
	$(MAKE) -C build

build: Makefile
	rm -rf build
	mkdir build
	(cd build; CC="$(CC) $(CFLAGS)" LDFLAGS="$(LDFLAGS)" LIBS="$(LDLIBS)" ../configure --build=$(CONF_BUILD) --host=$(CONF_HOST))

romfs:
	$(ROMFSINST) build/ethtool /bin/ethtool

clean:
	rm -rf build

