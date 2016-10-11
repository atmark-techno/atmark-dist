romfs:
	mkdir -p $(ROMFSDIR)/usr/lib/ruby/data
	$(ROMFSINST) lib/td /usr/lib/ruby/vendor_ruby/td
	$(ROMFSINST) lib/td-client.rb /usr/lib/ruby/vendor_ruby/td-client.rb
	$(ROMFSINST) data/ca-bundle.crt /usr/lib/ruby/data/ca-bundle.crt
