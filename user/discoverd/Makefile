
EXEC = discoverd
OBJS = discoverd.o udpip.o

ifeq ($(CONFIG_USER_FLATFSD_FLATFSD),y)
	ifeq ($(CONFIG_USER_FLATFSD_DISABLE_USR1),y)
		CFLAGS += -DENABLE_FLATFSD
	else
		CFLAGS += -DENABLE_FLATFSD_USR1
	endif
endif

ifeq ($(CONFIG_USER_SERI2ETH_SERI2ETH),y)
	CFLAGS += -DENABLE_SERI2ETH
endif

# CFLAGS += -D__DEBUG__

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(OBJS): discover_proto.h

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

