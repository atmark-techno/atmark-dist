#
# Makefile of gstreamer
#

SRC_DIR_$(CONFIG_USER_GSTREAMER_0_10) = gstreamer0.10
SRC_DIR_$(CONFIG_USER_GSTREAMER_1_0) += gstreamer1.0

all romfs clean distclean:
	for i in $(SRC_DIR_y) ; do make -C $$i $@ || exit $?; done
