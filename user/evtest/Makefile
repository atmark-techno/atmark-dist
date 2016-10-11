EXEC = evtest
OBJS = evtest.o

OBJDIR = obj
SRCDIR = evtest

all: prepare $(OBJDIR)/$(EXEC)

prepare:
	-mkdir -p $(OBJDIR)

$(OBJDIR)/$(EXEC): $(addprefix $(OBJDIR)/,$(OBJS))
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

romfs: all
	$(ROMFSINST) $(OBJDIR)/$(EXEC) /bin/$(EXEC)

clean:
	-rm -f $(OBJDIR)/* *.elf *.gdb
	-rmdir $(OBJDIR)

distclean: clean
	rm -f *~ *.orig
