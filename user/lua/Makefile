SRC_DIR = lua-5.2.1

all: build

build: $(SRC_DIR)/Makefile
	make linux -C $(SRC_DIR) 

romfs: build
	$(ROMFSINST) $(SRC_DIR)/src/lua /usr/bin/lua
	$(ROMFSINST) $(SRC_DIR)/src/luac /usr/bin/luac

clean:
	make -C $(SRC_DIR) clean
