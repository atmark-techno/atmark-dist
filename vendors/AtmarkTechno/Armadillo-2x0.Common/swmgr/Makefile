
EXEC = swmgr

SRCS = swmgr.c
OBJS = $(SRCS:.c=.o)

CFLAGS += -DEXEC_NAME=\"$(EXEC)\"

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	rm -f $(EXEC) $(OBJS)
