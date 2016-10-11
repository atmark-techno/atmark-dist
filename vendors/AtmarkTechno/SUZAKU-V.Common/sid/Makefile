UCLINUX_BUILD_USER=1
-include $(LINUX_CONFIG)
-include $(CONFIG_CONFIG)
-include $(ARCH_CONFIG)
-include $(MODULES_CONFIG)

TARGET_$(CONFIG_VENDOR_SUZAKU_SID_AD)     += demo-ad

CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/include

all: $(TARGET_y)

romfs:
	$(ROMFSINST) -e CONFIG_VENDOR_SUZAKU_SID_AD  /bin/demo-ad

clean:
	-rm -f $(TARGET_y) *.elf *.gdb *.o *~

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%: %.o
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

