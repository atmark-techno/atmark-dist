#
# Makefile of flatfsd
#

SRC_DIR_y =
SRC_DIR_$(CONFIG_USER_FLATFSD_3_0_1) += flatfsd-3.0.1
SRC_DIR_$(CONFIG_USER_FLATFSD_3_0_2) += flatfsd-3.0.2

all romfs clean distclean:
	for i in $(SRC_DIR_y) ; do make -C $$i $@ || exit $?; done
