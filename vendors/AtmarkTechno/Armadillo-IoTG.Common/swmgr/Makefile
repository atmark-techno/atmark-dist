EXEC = swmgr

SRCS = main.c
OBJS = $(SRCS:.c=.o)

CFLAGS += -DEXEC_NAME=\"$(EXEC)\" -Wall -Wextra

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	rm -f $(EXEC) $(OBJS)
