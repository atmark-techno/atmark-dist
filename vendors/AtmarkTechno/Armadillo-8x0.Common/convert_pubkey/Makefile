EXEC = convert_pubkey
OBJS = convert_pubkey.o

LDLIBS += -lssl -lcrypto

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

romfs:
	$(ROMFSINST) /usr/bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
