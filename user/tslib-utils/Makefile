#
#  tslib-utils/Makefile
#    based on tslib-1.0::tslib/tests/Makefile.am
#
#  Copyright (C) 2001 Russell King.
#  Copyright (C) 2010 Atmark Techno, Inc.
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#

LDLIBS += -lts

target-y =
target-$(CONFIG_USER_TSLIBUTILS_TS_CALIBRATE) += ts_calibrate
target-$(CONFIG_USER_TSLIBUTILS_TS_HARVEST) += ts_harvest
target-$(CONFIG_USER_TSLIBUTILS_TS_PRINT) += ts_print
target-$(CONFIG_USER_TSLIBUTILS_TS_PRINT_RAW) += ts_print_raw
target-$(CONFIG_USER_TSLIBUTILS_TS_TEST) += ts_test

all: $(target-y)

ts_calibrate: ts_calibrate.o fbutils.o testutils.o font_8x8.o font_8x16.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
ts_harvest: ts_harvest.o fbutils.o testutils.o font_8x8.o font_8x16.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
ts_print: ts_print.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
ts_print_raw: ts_print_raw.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
ts_test: ts_test.o fbutils.o font_8x8.o font_8x16.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

romfs:
	for target in $(target-y); do \
		$(ROMFSINST) /usr/bin/$$target; \
	done

clean:
	-rm -f ts_calibrate ts_harvest ts_print ts_print_raw ts_test
	-rm -f *.o
distclean: clean
	-rm -f *~
