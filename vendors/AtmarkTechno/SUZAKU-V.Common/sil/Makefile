UCLINUX_BUILD_USER=1
-include $(LINUX_CONFIG)
-include $(CONFIG_CONFIG)
-include $(ARCH_CONFIG)
-include $(MODULES_CONFIG)

TARGET_$(CONFIG_VENDOR_SUZAKU_SIL_LED)     += demo-led
TARGET_$(CONFIG_VENDOR_SUZAKU_SIL_7SEG)    += demo-7seg
TARGET_$(CONFIG_VENDOR_SUZAKU_SIL_SW)      += demo-sw
TARGET_$(CONFIG_VENDOR_SUZAKU_SIL_RSW)     += demo-rsw

all: $(TARGET_y)

romfs:
	$(ROMFSINST) -e CONFIG_VENDOR_SUZAKU_SIL_LED  /bin/demo-led
	$(ROMFSINST) -e CONFIG_VENDOR_SUZAKU_SIL_7SEG /bin/demo-7seg
	$(ROMFSINST) -e CONFIG_VENDOR_SUZAKU_SIL_SW   /bin/demo-sw
	$(ROMFSINST) -e CONFIG_VENDOR_SUZAKU_SIL_RSW  /bin/demo-rsw

clean:
	-rm -f $(TARGET_y) *.elf *.gdb *.o *~

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%: %.o
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

