-include hostbuild.mak

# Set to enable memory debugging commands
# CFLAGS += -DTCL_MEM_DEBUG

# A lot of work is done on the stack. If this is too small, we will just
# crash and burn
FLTFLAGS += -s 40000

EXE=tclsh
STATIC_LIBTCL=libtcl.a
CFLAGS += -Wall -g
ifdef CONFIG_LIB_TINYTCL_STATIC
LIBTCL=$(STATIC_LIBTCL)
else
LIBTCL=libtcl.so
CFLAGS += -fpic
endif
.EXPORT_ALL_VARIABLES:

OBJS=tclAssem.o tclGet.o tclBasic.o tclGlob.o tclUtil.o panic.o tclCmdAH.o tclHash.o tclCmdIL.o tclParse.o \
	regex_compat.o tclCmdMZ.o tclEnv.o tclProc.o tclVar.o tclalloc.o tclExpr.o tclunxaz.o tcluxutl.o tcluxstr.o \
	tclXdbg.o tclHistory.o

# Defines the extensions to include from extension/...
EXTENSIONS=syslog

# For dynamic loading
ifndef CONFIG_LIB_TINYTCL_STATIC
CFLAGS += -DHAVE_TCL_LOAD
LDFLAGS += -rdynamic
LIBDL += -ldl
OBJS += tclLoad.o tclLoadDl.o noextensions.o
else
OBJS += load_extensions.o
endif

ifdef CONFIG_USER_TINYTCL_DEBUGGER
# If you want to add the debugger...
OBJS += debugger/Dbg.o debugger/Dbg_cmd.o
CFLAGS += -I. -Idebugger -DDEBUGGER -DDEBUGGER_INIT=\"Dbg_lib.tcl\"
endif

TARGETS += $(LIBTCL)
ifdef UCLINUX_BUILD_USER
TARGETS += $(EXE)
endif

all: $(TARGETS)

$(EXE): $(LIBTCL) tinytcl.o
	$(PURIFY) $(CC) $(LDFLAGS) -o $@ tinytcl.o $(LIBTCL) $(LDLIBS$(LDLIBS_$@)) $(LIBDL)

tcltest: $(LIBTCL) tclTest.o
	$(PURIFY) $(CC) $(LDFLAGS) -o $@ tclTest.o $(LIBTCL) $(LDLIBS$(LDLIBS_$@)) $(LIBDL)

$(STATIC_LIBTCL): $(OBJS) build_extensions
	$(AR) cr $@ $(OBJS)
	-$(AR) r $@ extensions/*.o

libtcl.so: $(OBJS) build_extensions
	$(CC) -s -shared -o $@ $(SH_LDFLAGS) $(OBJS) $(SH_LDLIBS)

build_extensions:
	@for e in $(EXTENSIONS); do \
		$(MAKE) -C extensions/$$e TCLDIR=../.. -$(MAKEFLAGS) || exit 1; \
	done

load_extensions.c:
	@echo "#include \"tcl.h\"" > $@
	@for i in $(EXTENSIONS); do \
		echo "extern void $${i}_Init(Tcl_Interp *interp);" >>$@; \
	done
	@echo "void init_extensions(Tcl_Interp *interp) {" >> $@
	@for i in $(EXTENSIONS); do \
		echo "$${i}_Init(interp);" >>$@; \
	done
	@echo "}" >>$@

clean:
	rm -f *.o lib*.a lib*.so $(EXE) load_extensions.c tcltest extensions/*.o
	for e in $(EXTENSIONS); do \
		$(MAKE) -C extensions/$$e $@; \
	done

romfs:
	$(ROMFSINST) /bin/tclsh
ifndef CONFIG_LIB_TINYTCL_STATIC
	$(ROMFSINST) /lib/libtcl.so
	for e in $(EXTENSIONS); do \
		$(MAKE) -C extensions/$$e $@; \
	done
endif
