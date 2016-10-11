INSTDIR=/usr/lib/ruby/vendor_ruby/fluent/plugin
ROMFSINSTDIR=$(ROMFSDIR)$(INSTDIR)

romfs:
	mkdir -p $(ROMFSINSTDIR)
	$(ROMFSINST) lib/fluent/plugin/out_tditem.rb $(INSTDIR)/out_tditem.rb
	$(ROMFSINST) lib/fluent/plugin/out_tdlog.rb $(INSTDIR)/out_tdlog.rb
	$(ROMFSINST) lib/fluent/plugin/td_plugin_util.rb $(INSTDIR)/td_plugin_util.rb
	$(ROMFSINST) lib/fluent/plugin/td_plugin_version.rb $(INSTDIR)/td_plugin_version.rb
