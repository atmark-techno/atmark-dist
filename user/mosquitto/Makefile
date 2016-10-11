SRC_DIR = mosquitto-1.3.4

all: build

build: $(SRC_DIR)/Makefile
	make -C $(SRC_DIR) 

romfs: build
	$(ROMFSINST) -e CONFIG_USER_MOSQUITTO_MOSQUITTO_CLIENT $(SRC_DIR)/client/mosquitto_pub /usr/bin/mosquitto_pub
	$(ROMFSINST) -e CONFIG_USER_MOSQUITTO_MOSQUITTO_CLIENT $(SRC_DIR)/client/mosquitto_sub /usr/bin/mosquitto_sub

clean:
	make -C $(SRC_DIR) clean
