UCLINUX_BUILD_USER=1
-include $(LINUX_CONFIG)
-include $(CONFIG_CONFIG)
-include $(ARCH_CONFIG)
-include $(MODULES_CONFIG)

EXEC = 7seg-led-control.cgi
OBJS = 7seg-led-control.o


all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -Wall $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o *~

romfs:
	[ -d $(ROMFSDIR)/home/httpd ] || mkdir -p $(ROMFSDIR)/home/httpd
	$(ROMFSINST) /home/httpd/$(EXEC)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

