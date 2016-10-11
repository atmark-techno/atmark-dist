EXPECT_VERSION = 5.43.0
SRC_DIR = expect-$(EXPECT_VERSION)

MAKETARGET = $(CROSS_COMPILE:-=)

objs = exp_command.o expect.o pty_termios.o exp_inter.o exp_regexp.o \
       exp_tty.o exp_log.o exp_main_sub.o exp_pty.o exp_trap.o exp_console.o \
       exp_strf.o exp_glob.o exp_win.o exp_clib.o exp_closetcl.o \
       exp_memmove.o exp_tty_comm.o exp_select.o exp_event.o exp_chan.o Dbg.o

LIBEXPECT = libexpect.a
$(LIBEXPECT)_OBJ = exp_command.o expect.o pty_termios.o exp_inter.o \
		   exp_regexp.o exp_tty.o exp_log.o exp_main_sub.o \
		   exp_pty.o exp_trap.o exp_console.o exp_strf.o exp_glob.o \
		   exp_win.o exp_clib.o exp_closetcl.o exp_memmove.o \
		   exp_tty_comm.o exp_select.o exp_event.o exp_chan.o Dbg.o

EXEC = expect
$(EXEC)_OBJ = exp_main_exp.o $(LIBEXPECT)

CFLAGS += -I. -I/usr/$(MAKETARGET)/include/tcl8.4/tcl-private/generic \
	  -DEXP_VERSION="\"$(EXPECT_VERSION)\"" \
	  -DSCRIPTDIR=\"/usr/lib/expect5.43\" \
	  -DEXECSCRIPTDIR=\"/usr/lib/expect5.43\" \
	  -DTCL_DEBUGGER \
	  -DUSE_NON_CONST \
	  -DSTTY_BIN=\"/bin/stty\" \
	  -ansi \
	  -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_SOURCE \
	  -D_REENTRANT  -DDFLT_STTY="\"sane\""
LDLIBS += -ltcl8.4 -ldl -lpthread -lieee -lm -lutil

$(EXEC): $($(EXEC)_OBJ)
	$(CC) $(LDFLAGS) -o $@ $($(EXEC)_OBJ) $(LDLIBS)

$(LIBEXPECT): $($(LIBEXPECT)_OBJ)
	$(AR) cr $@ $($(LIBEXPECT)_OBJ)

%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

romfs: $(EXEC)
	$(ROMFSINST) /bin/$(EXEC)
	mkdir -p $(ROMFSDIR)/lib/tcl8.4
	$(ROMFSINST) /lib/tcl8.4/init.tcl

clean:
	rm -f *.o *.a $(EXEC)

distclean: clean
	rm -f *~ *.orig
