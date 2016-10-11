# Generated automatically from Makefile.in by configure.
#
prefix=$(PREFIX)
exec_prefix=${prefix}
sysconfdir=${prefix}/etc
sbindir=${exec_prefix}/sbin
INSTALL=/usr/bin/install -c

#
LM=dump_usbdev
CONF_DIR=${sysconfdir}/usbmgr
BIN_DIR=${sbindir}

BASE_DIR=..
INC_DIR=$(BASE_DIR)/inc
FILE_MODE=0555
SUB_DIR=tools

DEFS=-DCONF_DIR=\"$(CONF_DIR)\" -DVERSION=\"$(VERSION)\"
#DEBUG=-DDEBUG
INC=-I $(INC_DIR)
ifdef DEBUG
OPT=-g
endif

CFLAGS=${DEFS} ${OPT} ${INC} ${DEBUG}

all: $(LM)

clobber: clean clean_misc

clean_misc:
	rm -f Makefile

clean:
	rm -f $(LM)

install:
	$(INSTALL) -m $(FILE_MODE) $(LM) $(BIN_DIR)

romfs:
	$(ROMFSINST) $(BIN_DIR)/$(LM)
