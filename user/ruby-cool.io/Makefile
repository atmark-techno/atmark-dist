RUBY_CC_VERSION_$(CONFIG_USER_RUBY_RUBY_2_0_0_P451) = 2.0.0
RUBY_CC_VERSION_$(CONFIG_USER_RUBY_RUBY_2_1_2) = 2.1.2
RUBY_CC_VERSION = $(RUBY_CC_VERSION_y)

all: build

build: 
	rake RUBY_CC_VERSION=$(RUBY_CC_VERSION) CROSS_PLATFORM=$(CROSS_COMPILE:-=) cross compile

romfs:
	$(ROMFSINST) lib/cool.io /usr/lib/ruby/vendor_ruby/cool.io
	$(ROMFSINST) lib/cool.io.rb /usr/lib/ruby/vendor_ruby/cool.io.rb
	$(ROMFSINST) lib/coolio.rb /usr/lib/ruby/vendor_ruby/coolio.rb
	$(ROMFSINST) lib/cool.io_ext.so /usr/lib/ruby/vendor_ruby/cool.io_ext.so
	$(ROMFSINST) lib/http11_client.so /usr/lib/ruby/vendor_ruby/http11_client.so
	$(ROMFSINST) lib/iobuffer_ext.so /usr/lib/ruby/vendor_ruby/iobuffer_ext.so

clean:
	rm -rf ./tmp
	rake  RUBY_CC_VERSION=$(RUBY_CC_VERSION) CROSS_PLATFORM=$(CROSS_COMPILE:-=) cross clean
