#
# Makefile 
#   this file has been changed based on 1.41 2003/05/16 05:59:44 (David Hinds)
#

include $(LINUX_CONFIG)
include $(ARCH_CONFIG)

ifeq (config.mk, $(wildcard config.mk))
include config.mk
endif

ALL  = modules clients wireless cardmgr flash debug-tools man etc
DIRS = cardmgr flash debug-tools man etc
ifndef CONFIG_PCMCIA
DIRS := modules clients wireless $(DIRS)
endif


K_VERSION	=`grep "^VERSION" $(ROOTDIR)/$(LINUXDIR)/Makefile | \
			head -1 | sed -e "s/VERSION = //" `
K_PATCHLEVEL	=`grep "^PATCHLEVEL" $(ROOTDIR)/$(LINUXDIR)/Makefile | \
			head -1 | sed -e "s/PATCHLEVEL = //" `
K_SUBLEVEL	=`grep "^SUBLEVEL" $(ROOTDIR)/$(LINUXDIR)/Makefile | \
			head -1 | sed -e "s/SUBLEVEL = //" `
K_EXTRAVERSION	=`grep "^EXTRAVERSION" $(ROOTDIR)/$(LINUXDIR)/Makefile | \
			head -1 | sed -e "s/EXTRAVERSION = //" `
SRC_RELEASE=$(K_VERSION).$(K_PATCHLEVEL).$(K_SUBLEVEL)$(K_EXTRAVERSION)

CONFIGURE_OPTION = \
	--kernel=$(ROOTDIR)/$(LINUXDIR) \
	--target=$(ROOTDIR)/romfs \
	--moddir=/lib/modules/$(SRC_RELEASE) \
	--arch=$(ARCH) \
	--ucc=$(CC) \
	--kcc=$(CC) \
	--ld=$(LD) \
	--notrust \
	--nocardbus \
	--noprompt

all:	.prereq.ok kcheck
	@set -e ; for d in $(DIRS) ; do $(MAKE) -C $$d ; done
	@for f in *.mk ; do \
	    if [ $$f != config.mk -a $$f != rules.mk ] ; then \
	    $(MAKE) -f $$f all ; \
	fi ; done

help:
	@echo "Pick one of the following targets:"
	@echo -e "\tmake config\t\t- configure and check system setup"
	@echo -e "\tmake oldconfig\t\t- reconfigure without prompting"
	@echo -e "\tmake all\t\t- build modules and programs"
	@echo -e "\tmake install\t\t- install modules and programs"
	@echo -e "\tmake clean\t\t- remove old binaries and dependency files"
	@echo -e "\tmake realclean\t\t- start from scratch"

config .prereq.ok:
	if [ ! -d include/static ] ; then \
		mkdir include/static ; fi
	if [ ! -d include/static/pcmcia ] ; then \
		mkdir include/static/pcmcia ; fi
	if [ ! -e PCMCIA-HOWTO ] ; then \
		ln -s doc/PCMCIA-HOWTO PCMCIA-HOWTO ; fi
	if [ ! -e include/linux/compile.h ] ; then \
		ln -s ../pcmcia/config.h include/linux/compile.h ; fi
	if [ ! -e include/linux/config.h ] ; then \
		ln -s ../pcmcia/config.h include/linux/config.h ; fi
	if [ ! -e include/linux/version.h ] ; then \
		ln -s ../pcmcia/config.h include/linux/version.h ; fi
	if [ ! -e include/static/asm ] ; then \
		ln -s ../asm include/static/asm ; fi
	if [ ! -e include/static/linux ] ; then \
		ln -s ../linux include/static/linux ; fi
	if [ ! -e include/static/net ] ; then \
		ln -s ../net include/static/net ; fi
	if [ ! -e include/static/pcmcia/ciscode.h ] ; then \
		ln -s ../../pcmcia/ciscode.h include/static/pcmcia/ciscode.h ; fi
	if [ ! -e include/static/pcmcia/version.h ] ; then \
		ln -s ../../pcmcia/version.h include/static/pcmcia/version.h ; fi
	@touch config.mk
	@$(MAKE) -s clean
	@./Configure $(CONFIGURE_OPTION)

oldconfig:
	@touch config.mk
	@$(MAKE) -s clean
	@./Configure -n

clean:
	@touch config.mk
	@set -e ; for d in $(ALL) ; do $(MAKE) -C $$d clean ; done
	rm -f .prereq.ok config.mk include/pcmcia/autoconf.h
	rm -f include/linux/modversions.h

realclean:
	rm -f config.out
	@$(MAKE) clean

distclean: clean
	rm -f PCMCIA-HOWTO
	rm -f include/linux/compile.h
	rm -f include/linux/config.h
	rm -f include/linux/version.h
	rm -f include/static/asm
	rm -f include/static/linux
	rm -f include/static/net
	rm -f include/static/pcmcia/ciscode.h include/static/pcmcia/version.h
	rm -f config.out
	rm -rf include/static/pcmcia include/static

install: .prereq.ok kcheck
	@set -e ; for d in $(DIRS) ; do $(MAKE) -C $$d install ; done
	@for f in *.mk ; do \
	    if [ $$f != config.mk -a $$f != rules.mk ] ; then \
	    $(MAKE) -f $$f install ; \
	fi ; done
	@chmod 0744 scripts/depmod.pl
	scripts/depmod.pl -b $(PREFIX)/lib/modules/$(UTS_RELEASE) &> /dev/null

romfs: .prereq.ok kcheck
	for d in modules clients wireless cardmgr flash debug-tools etc ; \
	do $(MAKE) -C $$d install ; done
	@chmod 0744 scripts/depmod.pl
	scripts/depmod.pl -b $(PREFIX)/lib/modules/$(UTS_RELEASE) &> /dev/null

kcheck:
	@. ./config.out ; \
	if [ "$$CHECK" != "" ] ; then \
	    if [ "`cksum < $$CHECK`" != "$$CKSUM" ] ; then \
		make config ; \
	    fi ; \
	fi

