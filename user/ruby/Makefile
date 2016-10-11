SRC_DIR_$(CONFIG_USER_RUBY_RUBY_2_0_0_P451)	= ruby-2.0.0-p451
SRC_DIR_$(CONFIG_USER_RUBY_RUBY_2_1_2)	= ruby-2.1.2
SRC_DIR = $(SRC_DIR_y)

CONF_BUILD = $(shell $(SRC_DIR)/tool/config.guess)
CONF_HOST = $(shell $(SRC_DIR)/tool/config.sub $(CROSS:-=))

BUILD_DIR_$(CONFIG_USER_RUBY_RUBY_2_0_0_P451)	= builddir-2.0.0-p451
BUILD_DIR_$(CONFIG_USER_RUBY_RUBY_2_1_2)	= builddir-2.1.2
BUILD_DIR = $(BUILD_DIR_y)

DEST_DIR_$(CONFIG_USER_RUBY_RUBY_2_0_0_P451)	= destdir-2.0.0-p451
DEST_DIR_$(CONFIG_USER_RUBY_RUBY_2_1_2)	= destdir-2.1.2
DEST_DIR = $(DEST_DIR_y)

CONFIGURE_OPT = --build=$(CONF_BUILD) \
		--host=$(CONF_HOST)   \
		--prefix=/usr         \
		--enable-shared       \
		--enable-ipv6

all: build

$(BUILD_DIR)/Makefile:
	mkdir -p $(BUILD_DIR)
	(cd $(BUILD_DIR); ../$(SRC_DIR)/configure $(CONFIGURE_OPT))

build: $(BUILD_DIR)/Makefile
	make -C $(BUILD_DIR) main
	make DESTDIR=../$(DEST_DIR) -C $(BUILD_DIR) install-nodoc
	rm -rf $(DEST_DIR)/usr/lib/ruby/gems

romfs: build
	$(ROMFSINST) $(DEST_DIR)/usr/bin/ruby /usr/bin/
	$(ROMFSINST) $(DEST_DIR)/usr/lib/ruby /usr/lib/ruby
ifdef CONFIG_USER_RUBY_RUBY_2_0_0_P451
	$(ROMFSINST) $(DEST_DIR)/usr/lib/libruby.so.2.0.0 /usr/lib/
	$(ROMFSINST) -s /usr/lib/libruby.so.2.0.0 /usr/lib/libruby.so.2.0
	$(ROMFSINST) -s /usr/lib/libruby.so.2.0 /usr/lib/libruby.so.2
endif
ifdef CONFIG_USER_RUBY_RUBY_2_1_2
	$(ROMFSINST) $(DEST_DIR)/usr/lib/libruby.so.2.1.0 /usr/lib/
	$(ROMFSINST) -s /usr/lib/libruby.so.2.1.0 /usr/lib/libruby.so.2.1
	$(ROMFSINST) -s /usr/lib/libruby.so.2.1 /usr/lib/libruby.so.2
endif

clean:
	rm -rf $(DEST_DIR)
	make -C $(BUILD_DIR) clean

distclean: clean
	rm -rf $(BUILD_DIR)
