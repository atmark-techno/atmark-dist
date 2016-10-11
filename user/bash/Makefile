
SRC_DIR_$(CONFIG_USER_BASH_BASH_2)  = bash-2.05
SRC_DIR_$(CONFIG_USER_BASH_BASH_4)  = bash-4.3.33
SRC_DIR = $(SRC_DIR_y)

build:
	make -C $(SRC_DIR)

romfs: build
	make -C $(SRC_DIR) romfs

clean:
	make -C $(SRC_DIR) clean

distclean:
	make -C $(SRC_DIR) distclean
