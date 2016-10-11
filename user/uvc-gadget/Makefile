EXEC = uvc-gadget
OBJS = uvc-gadget.o

CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/include

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

romfs:
	$(ROMFSINST) /usr/sbin/uvc-gadget
	$(ROMFSINST) /etc

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
