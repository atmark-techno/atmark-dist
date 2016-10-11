### 
### I think it's not worth to make such a small project
### modular. So this is a simple gnu Makefile...
###
EXEC = fbgrab
OBJS = fbgrab.o

CFLAGS += -I$(ROOTDIR)/lib/libpng
LDFLAGS += -L$(ROOTDIR)/lib/libpng
LDLIBS += -lpng -lz

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *~ \#*\# *.o
