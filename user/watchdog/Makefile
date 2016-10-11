
EXEC = watchdog 
OBJS = watchdog.o
SRCS = watchdog.c

# CFLAGS += -DWDT_DEBUG

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -c -o $@ $<

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o


