RUBY_CC_VERSION_$(CONFIG_USER_RUBY_RUBY_2_0_0_P451) = 2.0.0
RUBY_CC_VERSION_$(CONFIG_USER_RUBY_RUBY_2_1_2) = 2.1.2
RUBY_CC_VERSION = $(RUBY_CC_VERSION_y)

all: build

build: 
	rake RUBY_CC_VERSION=$(RUBY_CC_VERSION) CROSS_PLATFORM=$(CROSS_COMPILE:-=) cross compile

romfs:
	$(ROMFSINST) lib/string /usr/lib/ruby/vendor_ruby/string
	$(ROMFSINST) lib/string-scrub.rb /usr/lib/ruby/vendor_ruby/string-scrub.rb

clean:
	rm -rf ./tmp
	rake  RUBY_CC_VERSION=$(RUBY_CC_VERSION) CROSS_PLATFORM=$(CROSS_COMPILE:-=) cross clean
