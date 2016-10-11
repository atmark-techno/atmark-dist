EXEC = camctrl
OBJS = camctrl.o sccb.o ov7725.o
HEADERS = camctrl.h armadillo.h sccb.h ov7725.h version.h

LDLIBS += $(shell pkg-config --libs glib-2.0)
CFLAGS += $(shell pkg-config --cflags glib-2.0) -I$(ROOTDIR)/include

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJS): $(HEADERS)

romfs:
	$(ROMFSINST) /usr/bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o *~
