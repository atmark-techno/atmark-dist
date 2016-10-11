#
# Makefile of udev
#

SRC_DIR_y = 
SRC_DIR_$(CONFIG_USER_UDEV_105) += udev-105
SRC_DIR_$(CONFIG_USER_UDEV_175) += udev-175

all romfs clean distclean:
	for i in $(SRC_DIR_y) ; do make -C $$i $@ || exit $?; done
