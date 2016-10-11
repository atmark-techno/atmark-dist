
SRC_DIR_$(CONFIG_USER_MJPGSTREAMER94_MJPGSTREAMER)  = mjpg-streamer.r94
SRC_DIR_$(CONFIG_USER_MJPGSTREAMER182_MJPGSTREAMER) = mjpg-streamer.r182
SRC_DIR = $(SRC_DIR_y)

shlibs = input_control.so input_file.so input_uvc.so \
	 output_file.so output_http.so

CFLAGS = 

build:
	make -C $(SRC_DIR) CC=$(CROSS_COMPILE)gcc

romfs: build
	mkdir -p $(ROMFSDIR)/usr
	mkdir -p $(ROMFSDIR)/usr/bin
	mkdir -p $(ROMFSDIR)/usr/lib
	mkdir -p $(ROMFSDIR)/usr/lib/mjpg_streamer
	for target in $(shlibs); do \
		$(ROMFSINST) $(SRC_DIR)/$$target \
		             /usr/lib/mjpg_streamer/$$target; \
	done
	$(ROMFSINST) $(SRC_DIR)/mjpg_streamer /usr/bin/mjpg_streamer
	$(ROMFSINST) $(SRC_DIR)/www /usr/lib/mjpg_streamer/www

clean:
	make -C $(SRC_DIR) clean
