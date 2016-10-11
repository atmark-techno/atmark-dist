RUBY_CC_VERSION_$(CONFIG_USER_RUBY_RUBY_2_0_0_P451) = 2.0.0
RUBY_CC_VERSION_$(CONFIG_USER_RUBY_RUBY_2_1_2) = 2.1.2
RUBY_CC_VERSION = $(RUBY_CC_VERSION_y)

all: build

build: 
	rake RUBY_CC_VERSION=$(RUBY_CC_VERSION) CROSS_PLATFORM=$(CROSS_COMPILE:-=) cross compile

romfs:
	$(ROMFSINST) lib/http /usr/lib/ruby/vendor_ruby/http
	$(ROMFSINST) lib/http_parser.rb /usr/lib/ruby/vendor_ruby/http_parser.rb
	$(ROMFSINST) lib/ruby_http_parser.so /usr/lib/ruby/vendor_ruby/ruby_http_parser.so

clean:
	rm -rf ./tmp
	rake  RUBY_CC_VERSION=$(RUBY_CC_VERSION) CROSS_PLATFORM=$(CROSS_COMPILE:-=) cross clean
