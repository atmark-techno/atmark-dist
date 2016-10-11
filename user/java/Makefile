export JAVA_HOME=$(shell ls -d /usr/lib/jvm/java-7-openjdk-*)
EJDKDIR=$(ROOTDIR)/ejdk
JRE=$(EJDKDIR)/jre
JRECREATE_OPT = --dest $(JRE)
ifneq ($(CONFIG_USER_JAVA_SE8_PROFILE_FULL),y)
	JRE_PROFILE_$(CONFIG_USER_JAVA_SE8_PROFILE_COMPACT1) = compact1
	JRE_PROFILE_$(CONFIG_USER_JAVA_SE8_PROFILE_COMPACT2) = compact2
	JRE_PROFILE_$(CONFIG_USER_JAVA_SE8_PROFILE_COMPACT3) = compact3
	JRE_PROFILE = $(JRE_PROFILE_y)
	JRECREATE_OPT += --profile $(JRE_PROFILE)
endif

JRE_VM_$(CONFIG_USER_JAVA_SE8_VM_MINIMAL) = minimal
JRE_VM_$(CONFIG_USER_JAVA_SE8_VM_CLIENT) = client
JRE_VM_$(CONFIG_USER_JAVA_SE8_VM_SERVER) = server
JRE_VM_$(CONFIG_USER_JAVA_SE8_VM_ALL) = all
JRE_VM = $(JRE_VM_y)
JRECREATE_OPT += --vm $(JRE_VM)
ifeq ($(CONFIG_USER_JAVA_SE8_EXT_SUNEC),y)
	JRECREATE_EXT := sunec,
endif
ifeq ($(CONFIG_USER_JAVA_SE8_EXT_SUNPKCS11),y)
	JRECREATE_EXT := $(JRECREATE_EXT)sunpkcs11,
endif
ifeq ($(CONFIG_USER_JAVA_SE8_EXT_LOCALES),y)
	JRECREATE_EXT := $(JRECREATE_EXT)locales,
endif
ifeq ($(CONFIG_USER_JAVA_SE8_EXT_CHARSETS),y)
	JRECREATE_EXT := $(JRECREATE_EXT)charsets,
endif
ifeq ($(CONFIG_USER_JAVA_SE8_EXT_NASHORN),y)
	JRECREATE_EXT := $(JRECREATE_EXT)nashorn,
endif
FX_EXTENSIONS = $(shell find $(EJDKDIR)/ -type d | grep "extensions/fx")
ifeq ($(CONFIG_USER_JAVA_SE8_EXT_FXGRAPHICS),y)
ifneq ($(FX_EXTENSIONS),)
	JRECREATE_EXT := $(JRECREATE_EXT)fx:graphics,
endif
endif

ifeq ($(CONFIG_USER_JAVA_SE8_EXT_FXCONTROLS),y)
ifneq ($(FX_EXTENSIONS),)
	JRECREATE_EXT := $(JRECREATE_EXT)fx:controls,
endif
endif
ifneq ($(JRECREATE_EXT),)
	JRECREATE_OPT += --extension $(JRECREATE_EXT)
endif

all: $(JRE)

$(JRE): $(EJDKDIR)
	$(EJDKDIR)/bin/jrecreate.sh $(JRECREATE_OPT)

$(EJDKDIR):
	@echo 'Please place $(EJDKDIR) directory for Java Rumtime.'; \
	exit 1;

romfs: $(JRE)
	$(ROMFSINST) $(JRE) / 

clean:
	-rm -rf $(JRE)

distclean:
	-rm -rf $(JRE)
