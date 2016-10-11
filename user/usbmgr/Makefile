# Generated automatically from Makefile.in by configure.
#
# Makefile for usbmgr
#                               <shuu@dotAster.com>
#
# Checking SUSE and installing from Duncan Bellamy.
#
prefix=$(PREFIX)
exec_prefix=${prefix}
sysconfdir=${prefix}/etc
sbindir=${exec_prefix}/sbin
INSTALL=/usr/bin/install -c

LM=usbmgr
UPDATE=update_usbdb
CONF_DIR=${sysconfdir}/usbmgr
CONF_FILE=usbmgr.conf
PRELOAD=preload.conf
BIN_DIR=${sbindir}

## rc file
RC_INIT_DIR=${sysconfdir}/rc.d/init.d
REDHAT_FILE_CHECK=$(RC_INIT_DIR)/functions
SUSE_RC_DIR=${sysconfdir}/init.d
SUSE_FILE_CHECK=${sysconfdir}/rc.config
SCRIPT_DIR=scripts
RC_FILE=rcscript
SUSE_RC_FILE=rcusbmgr
NETWORK_SCRIPT=network
SCRIPTS=$(NETWORK_SCRIPT)

## install
#FILE_MODE=0444
FILE_MODE=0666
#BIN_MODE=0555
BIN_MODE=0777
#SCRIPT_MODE=0755
SCRIPT_MODE=0777
INSTALL_DIR=$(INSTALL) -d
INSTALL_FILE=$(INSTALL) -m $(FILE_MODE)
INSTALL_BIN=$(INSTALL) -m $(BIN_MODE)
INSTALL_SCRIPT=$(INSTALL) -m $(SCRIPT_MODE)

BASE_DIR=.
INC_DIR=$(BASE_DIR)/inc
MAJOR_VERSION=1
MINOR_VERSION=0
PATCH_LEVEL=0
VERSION=$(MAJOR_VERSION).$(MINOR_VERSION).$(PATCH_LEVEL)
SUB_DIR=tools

DEFS=-DCONF_DIR=\"$(CONF_DIR)\" -DCONF_FILE=\"$(CONF_FILE)\" -DPRELOAD=\"$(PRELOAD)\" -DBIN_DIR=\"$(BIN_DIR)\" -DVERSION=\"$(VERSION)\" -DNETWORK_SCRIPT=\"$(NETWORK_SCRIPT)\" -DWAIT_FLUSH_PROC
#DEBUG=-DDEBUG
#DEBUG=-DDEBUG -DDEBUG_ALLOC
#DEBUG=-DDEBUG -DDEVFILE=\"/usr/src/usbmgr/usbmgr/devices\"
INC=-I $(INC_DIR)
OPT=-O2 -Wall
ifdef DEBUG
OPT=-g
endif

########################################## Don't modify 

CFLAGS=$(DEFS) $(OPT) $(INC) $(DEBUG)

USBMGR_SRC=node.c loader.c device.c parse_device.c usbmgr_msg.c module.c beep.c
UPDATE_SRC=node.c parse_config.c config.c modlink.c misc.c module.c
ifdef DEBUG
USBMGR_SRC := debug.c $(USBMGR_SRC)
UPDATE_SRC := debug.c $(UPDATE_SRC)
endif
USBMGR_OBJ=$(USBMGR_SRC:.c=.o)
UPDATE_OBJ=$(UPDATE_SRC:.c=.o)

all: $(LM) $(UPDATE)
	@for d in $(SUB_DIR);\
	do \
		cd $$d;make VERSION=${VERSION};cd ..;\
	done

$(LM): main.o $(USBMGR_OBJ)
	$(CC) -o $@ main.o $(USBMGR_OBJ)

$(UPDATE): $(UPDATE).o $(UPDATE_OBJ)
	$(CC) -o $@ $(UPDATE).o $(UPDATE_OBJ)

clobber: clean
	rm -f config.cache config.log config.status #config.h Makefile 
	@for d in $(SUB_DIR); \
	do \
		cd $$d;make clobber;cd ..;\
	done

clean_misc:
	rm -f config.cache config.log config.status #config.h Makefile
	@for d in $(SUB_DIR); \
	do \
		cd $$d;make clean_misc;cd ..;\
	done

clean:
	rm -f main.o update_usbdb.o  $(USBMGR_OBJ) $(UPDATE_OBJ) debug.o $(LM) $(UPDATE)
	@for d in $(SUB_DIR); \
	do \
		cd $$d;make clean;cd ..;\
	done

install:
	$(INSTALL_DIR) $(BIN_DIR)
	$(INSTALL_DIR) $(CONF_DIR)
	@if [ -f $(CONF_DIR)/$(CONF_FILE) ];then \
		mv $(CONF_DIR)/$(CONF_FILE) $(CONF_DIR)/$(CONF_FILE).old; \
	fi
	$(INSTALL_FILE) config/$(CONF_FILE) $(CONF_DIR)
	$(INSTALL_BIN) $(LM) $(BIN_DIR)
	$(INSTALL_BIN) $(UPDATE) $(BIN_DIR)
	@if [ -e $(SUSE_FILE_CHECK) ];then \
		if [ -f $(SUSE_RC_DIR)/usbmgr ];then \
			mv $(SUSE_RC_DIR)/usbmgr $(SUSE_RC_DIR)/usbmgr.old; \
		fi;\
		$(INSTALL_SCRIPT) $(SCRIPT_DIR)/$(SUSE_RC_FILE) $(SUSE_RC_DIR)/usbmgr;\
		ln -sf ../$(SUSE_RC_DIR)/usbmgr $(BIN_DIR)/rcusbmgr; \
	elif [ -d $(RC_INIT_DIR) ];then \
		if [ -f $(RC_INIT_DIR)/usbmgr ];then \
			mv $(RC_INIT_DIR)/usbmgr $(RC_INIT_DIR)/usbmgr.old; \
		fi;\
		$(INSTALL_SCRIPT) $(SCRIPT_DIR)/$(RC_FILE) $(RC_INIT_DIR)/usbmgr;\
		ln -sf ../init.d/usbmgr $(RC_INIT_DIR)/../rc5.d/S46usbmgr; \
	else \
		echo '##############################'; \
		echo '# You should setup RC script #'; \
		echo '#  for usbmgr by yourself.   #'; \
		echo '##############################'; \
	fi
	@for f in $(SCRIPTS);\
	do \
		if [ -f $(CONF_DIR)/$$f ];then \
			mv $(CONF_DIR)/$$f $(CONF_DIR)/$$f.old;\
		fi;\
		$(INSTALL_SCRIPT) $(SCRIPT_DIR)/$$f $(CONF_DIR);\
	done
	@for d in $(SUB_DIR);\
	do \
		cd $$d;make install;cd ..;\
	done
	@echo "###############################################"
	@echo "If you use USB mice on X, Run \"make use_mouse\"!"
	@echo "###############################################"

romfs:
	mkdir -p $(ROMFSDIR)/$(BIN_DIR)
	mkdir -p $(ROMFSDIR)/$(CONF_DIR)
	@if [ ! -f $(ROMFSDIR)/$(CONF_DIR)/$(CONF_FILE) ];then \
		$(ROMFSINST) -p $(FILE_MODE) config/$(CONF_FILE) $(CONF_DIR)/$(CONF_FILE);\
	fi
	$(ROMFSINST) -p $(BIN_MODE) $(BIN_DIR)/$(LM)
	$(ROMFSINST) -p $(BIN_MODE) $(BIN_DIR)/$(UPDATE)
	@if [ -e $(SUSE_FILE_CHECK) ];then \
		$(ROMFSINST) -p $(SCRIPT_MODE) $(SCRIPT_DIR)/$(SUSE_RC_FILE) $(SUSE_RC_DIR)/usbmgr/$(SUSE_RC_FILE);\
		$(ROMFSINST) -s ../$(SUSE_RC_DIR)/usbmgr $(BIN_DIR)/rcusbmgr; \
	elif [ -d $(RC_INIT_DIR) ];then \
		$(ROMFIINST) -p $(SCRIPT_MODE) $(SCRIPT_DIR)/$(RC_FILE) $(RC_INIT_DIR)/usbmgr/$(RC_FILE);\
		$(ROMFSINST) -s ../init.d/usbmgr $(RC_INIT_DIR)/../rc5.d/S46usbmgr; \
	fi
	@for f in $(SCRIPTS);\
	do \
		$(ROMFSINST) -p $(SCRIPT_MODE) $(SCRIPT_DIR)/$$f $(CONF_DIR)/$$f;\
	done
	@for d in $(SUB_DIR);\
	do \
		cd $$d;make romfs;cd ..;\
	done

use_mouse:
	@if [ -f $(CONF_DIR)/$(PRELOAD) ];then \
		mv $(CONF_DIR)/$(PRELOAD) $(CONF_DIR)/$(PRELOAD).old; \
	fi
	$(INSTALL_FILE) config/$(PRELOAD) $(CONF_DIR)
