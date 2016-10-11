PROG = bplay
OBJS = bplay.o sndfunc.o shmbuf.o
SRCS = bplay.c sndfunc.o shmbuf.c

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PROG) $(LIBS) $(LDLIBS)
	ln -sf bplay brec

romfs:
	$(ROMFSINST) -e CONFIG_USER_BPLAY_BPLAY /bin/$(PROG)
	$(ROMFSINST) -e CONFIG_USER_BPLAY_BPLAY -s /bin/$(PROG) /bin/brec

clean:
	rm -f bplay brec *.o *~ *.gdb
