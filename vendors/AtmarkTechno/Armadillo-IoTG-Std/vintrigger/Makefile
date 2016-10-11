EXEC = vintrigger
OBJS = vintrigger.o

CFLAGS += -idirafter $(ROOTDIR)/$(LINUXDIR)/include

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs:
	$(ROMFSINST) /usr/bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
