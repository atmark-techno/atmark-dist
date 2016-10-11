SRC_DIR = alsa-utils

target_y :=
target_$(CONFIG_USER_ALSAUTILS_APLAY)	+= aplay
target_$(CONFIG_USER_ALSAUTILS_ARECORD)	+= aplay
target_$(CONFIG_USER_ALSAUTILS_AMIXER)	+= amixer

aplay_obj = aplay.o
amixer_obj = amixer.o

CFLAGS += -I$(shell pwd)/$(SRC_DIR)/include
LDLIBS += -lasound -lm

all: prepare $(target_y)

prepare: $(SRC_DIR)/include/aconfig.h $(SRC_DIR)/include/version.h
$(SRC_DIR)/include/aconfig.h:
	cp aconfig.h $(SRC_DIR)/include/
$(SRC_DIR)/include/version.h:
	cp version.h $(SRC_DIR)/include/

aplay: $(aplay_obj)
	$(CC) $(LDFLAGS) -o $@ $(aplay_obj) $(LDLIBS)

aplay.o: $(SRC_DIR)/aplay/aplay.c
	$(CC) $(CFLAGS) -c -o $@ $<

arecord: aplay

amixer: $(amixer_obj)
	$(CC) $(LDFLAGS) -o $@ $(amixer_obj) $(LDLIBS)

amixer.o: $(SRC_DIR)/amixer/amixer.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o aplay amixer
	rm -f $(SRC_DIR)/include/aconfig.h $(SRC_DIR)/include/version.h

romfs:
	$(ROMFSINST) -e CONFIG_USER_ALSAUTILS_APLAY /bin/aplay
	$(ROMFSINST) -e CONFIG_USER_ALSAUTILS_ARECORD /bin/aplay
	$(ROMFSINST) -e CONFIG_USER_ALSAUTILS_ARECORD \
			-s /bin/aplay /bin/arecord
	$(ROMFSINST) -e CONFIG_USER_ALSAUTILS_AMIXER /bin/amixer
