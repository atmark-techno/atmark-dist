#
# Makefile of iptables
#

ifeq ($(CONFIG_USER_IPTABLES_IPTABLES_1_3_6),y)

SRC_DIR=iptables-1.3.6

all romfs clean distclean:
	for i in $(SRC_DIR) ; do make -C $$i $@ || exit $?; done

else

all romfs clean distclean:
	make -f iptables-1.4.x.mak $@

endif
