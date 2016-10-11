UCLINUX_BUILD_USER=1
-include $(LINUX_CONFIG)
-include $(CONFIG_CONFIG)
-include $(ARCH_CONFIG)
-include $(MODULES_CONFIG)

TARGET_$(CONFIG_VENDOR_SUZAKU_SIV_AV)         += demo-av
TARGET_$(CONFIG_VENDOR_SUZAKU_SIV_IMPOSE)     += demo-impose

CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/include

all: $(TARGET_y)

romfs:
	[ -d $(ROMFSDIR)/home/httpd ] || mkdir -p $(ROMFSDIR)/home/httpd
	$(ROMFSINST) -e CONFIG_VENDOR_SUZAKU_SIV_AV demo-av /home/httpd/demo-av.cgi
	$(ROMFSINST) -e CONFIG_VENDOR_SUZAKU_SIV_IMPOSE /bin/demo-impose

clean:
	-rm -f $(TARGET_y) *.elf *.gdb *.o *~

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%: %.o
	$(CC) -Wall $(LDFLAGS) -o $@ $< $(LDLIBS)
