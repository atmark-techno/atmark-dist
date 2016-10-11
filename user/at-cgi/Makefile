#
# This file is part of at-cgi
#
ifeq ($(CONFIG_USER_BUSYBOX_1_20_2),y)
include $(ROOTDIR)/user/busybox/busybox-1.20.2/.config
endif

VPATH= simple-cgi simple-cgi-app at-cgi/common at-cgi/modules

SIMPLECGIOBJ= simple-cgi.o
SIMPLECGITESTOBJ= simple-cgi-test.o
SIMPLECGIAPPOBJ= simple-cgi-app-html-parts.o simple-cgi-app-misc.o \
	simple-cgi-app-str.o simple-cgi-app-alloc.o simple-cgi-app-io.o \
	simple-cgi-app-llist.o
ATCGICOMMONOBJ= common.o frame-html.o menu.o misc-utils.o passwd.o system-config.o
OVERVIEWOBJ= overview_main.o overview_display.o
SYSTEMOBJ= system_main.o system_display.o
USBDATAOBJ= usbdata_main.o usbdata_display.o
PACKETSCANOBJ= packetscan_main.o packetscan_display.o packetscan_common.o

##

EXEC= index.cgi

ifdef CONFIG_USER_AT_CGI_TOPONLY

CFLAGS += -DINDEX_ONLY

else

ifdef CONFIG_USER_AT_CGI_SYSTEM

EXEC += system.cgi
OVERVIEW_REFS += system.cgi
CFLAGS += -DSYSTEM

ifdef CONFIG_USER_AT_CGI_FIREWALL
CFLAGS += -DFIREWALL
endif

ifdef CONFIG_USER_AT_CGI_BRIDGE
CFLAGS += -DBRIDGE
endif

endif

ifeq ($(filter y,$(CONFIG_USER_BUSYBOX_SYSLOGD) $(CONFIG_SYSLOGD)),y)
CFLAGS += -DSYSLOGD
endif

ifdef CONFIG_USER_AT_CGI_USBDATA
EXEC += usbdata.cgi
OVERVIEW_REFS += usbdata.cgi
CFLAGS += -DUSBDATA
endif

ifdef CONFIG_USER_AT_CGI_PACKETSCAN
EXEC += packetscan.cgi
OVERVIEW_REFS += packetscan.cgi
CFLAGS += -DPACKETSCAN
endif

endif

##

all: $(EXEC)

$(OVERVIEWOBJ): $(OVERVIEW_REFS)

libsimplecgi.a: $(SIMPLECGIOBJ)
	$(AR) cru libsimplecgi.a $(SIMPLECGIOBJ)

simple-cgi-test.cgi: $(SIMPLECGITESTOBJ) libsimplecgi.a
	$(CC) $(LDFLAGS) -o $@ $(SIMPLECGITESTOBJ) -lsimplecgi -L .

libsimplecgiapp.a: $(SIMPLECGIAPPOBJ)
	$(AR) cru libsimplecgiapp.a $(SIMPLECGIAPPOBJ)

libatcgicommon.a: $(ATCGICOMMONOBJ)
	$(AR) cru libatcgicommon.a $(ATCGICOMMONOBJ)

index.cgi: $(OVERVIEWOBJ) packetscan_common.o libatcgicommon.a libsimplecgiapp.a libsimplecgi.a
	$(CC) $(LDFLAGS) -o $@ $(OVERVIEWOBJ) packetscan_common.o -latcgicommon -lsimplecgiapp -lsimplecgi -L .

system.cgi: $(SYSTEMOBJ) libatcgicommon.a libsimplecgiapp.a libsimplecgi.a
	$(CC) $(LDFLAGS) -o $@ $(SYSTEMOBJ) $(LIBCRYPT) -latcgicommon -lsimplecgiapp -lsimplecgi -L .

usbdata.cgi: $(USBDATAOBJ) libatcgicommon.a libsimplecgiapp.a libsimplecgi.a
	$(CC) $(LDFLAGS) -o $@ $(USBDATAOBJ) -latcgicommon -lsimplecgiapp -lsimplecgi -L .

packetscan.cgi: $(PACKETSCANOBJ) libatcgicommon.a libsimplecgiapp.a libsimplecgi.a
	$(CC) $(LDFLAGS) -o $@ $(PACKETSCANOBJ) -latcgicommon -lsimplecgiapp -lsimplecgi -L .

romfs:
	mkdir -p $(ROMFSDIR)/home
	mkdir -p $(ROMFSDIR)/home/www-data
	mkdir -p $(ROMFSDIR)/home/www-data/errors
	mkdir -p $(ROMFSDIR)/home/www-data/images
	mkdir -p $(ROMFSDIR)/home/www-data/admin
	mkdir -p $(ROMFSDIR)/tmp
	mkdir -p $(ROMFSDIR)/tmp/at-admin

	$(ROMFSINST) at-cgi/html/errors /home/www-data/errors
	$(ROMFSINST) at-cgi/html/style.css /home/www-data/style.css
	$(ROMFSINST) at-cgi/html/redirect.html /home/www-data/admin/index.html
	$(ROMFSINST) at-cgi/images /home/www-data/images
	chmod 644 $(ROMFSDIR)/home/www-data/images/*
	$(ROMFSINST) /home/www-data/index.cgi
ifdef CONFIG_USER_AT_CGI_SYSTEM
	$(ROMFSINST) /home/www-data/admin/system.cgi
	chmod 755 $(ROMFSDIR)/home/www-data/admin/system.cgi
endif
ifdef CONFIG_USER_AT_CGI_USBDATA
	$(ROMFSINST) /home/www-data/usbdata.cgi
	chmod 755 $(ROMFSDIR)/home/www-data/usbdata.cgi
endif
ifdef CONFIG_USER_AT_CGI_PACKETSCAN
	$(ROMFSINST) /home/www-data/admin/packetscan.cgi
	chmod 755 $(ROMFSDIR)/home/www-data/admin/packetscan.cgi
endif

clean:
	-rm -f *.elf *.gdb *.o *.a *.cgi *~

%.o: %.c
	$(CC) -c $(CFLAGS) -I ./simple-cgi -I ./simple-cgi-app -I ./at-cgi/common -o $@ $<
