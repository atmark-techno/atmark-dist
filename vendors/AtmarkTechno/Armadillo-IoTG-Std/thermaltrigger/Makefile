EXEC = thermaltrigger
OBJS = thermaltrigger.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs:
	$(ROMFSINST) /usr/bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
