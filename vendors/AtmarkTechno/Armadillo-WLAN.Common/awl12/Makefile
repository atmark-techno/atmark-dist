AERIALDIR = $(ROOTDIR)/aerial
MODULE = $(AERIALDIR)/src/aerial.ko

all: $(MODULE)

$(MODULE): $(AERIALDIR)
	make -C $(AERIALDIR) KERNELDIR=$(ROOTDIR)/$(LINUXDIR)

$(AERIALDIR):
	@echo 'Please place $(AERIALDIR) directory for Armadillo-WLAN (AWL12).'; \
	exit 1;

romfs: $(MODULE)
	make -C $(AERIALDIR) KERNELDIR=$(ROOTDIR)/$(LINUXDIR) INSTALL_MOD_PATH=$(ROMFSDIR) install

clean:
	-make -C $(AERIALDIR) KERNELDIR=$(ROOTDIR)/$(LINUXDIR) clean

distclean:
	-make -C $(AERIALDIR) KERNELDIR=$(ROOTDIR)/$(LINUXDIR) distclean
