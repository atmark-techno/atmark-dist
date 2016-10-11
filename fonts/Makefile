#
#	Makefile -- Build instructions for Fonts
#

-include $(CONFIG_CONFIG)

fontdir_y =
fontdir_$(CONFIG_FONTS_TAKAO) += truetype/fonts-takao
fontdir_$(CONFIG_FONTS_VL) += truetype/vlgothic

all clean:
# nothing to do here

romfs:
	for i in $(sort $(fontdir_y)); do \
		[ ! -d $$i ] || make -C $$i romfs || exit $$?; \
	done
