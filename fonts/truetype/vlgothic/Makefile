#
#	Makefile -- Takao Fonts
#

-include $(CONFIG_CONFIG)

empty =
space = $(empty) $(empty)
escape = $(empty)\$(empty)
fakespace = @

file_y += LICENSE LICENSE.en LICENSE_E.mplus LICENSE_J.mplus
file_y += README README.sazanami README_J.mplus
file_$(CONFIG_FONTS_VL_GOTHIC) += VL-Gothic-Regular.ttf
file_$(CONFIG_FONTS_VL_PGOTHIC) += VL-PGothic-Regular.ttf

family_$(CONFIG_FONTS_VL_GOTHIC) += VL$(fakespace)ゴシック VL$(fakespace)Gothic
family_$(CONFIG_FONTS_VL_PGOTHIC) += VL$(fakespace)Pゴシック VL$(fakespace)PGothic

all:
# nothing to do here

romfs:
	mkdir -p $(ROMFSDIR)/usr/share/fonts/truetype/vlgothic/
	for i in $(file_y); do \
		$(ROMFSINST) $$i /usr/share/fonts/truetype/vlgothic/$$i; \
	done

	mkdir -p $(ROMFSDIR)/etc/fonts/conf.d/
	if [ "$(family_)" ]; then \
		cat 66-vlgothic-fonts.conf | \
		grep -v $(subst $(fakespace),$(escape)$(space),$(addprefix -e ,$(family_))) > $(ROMFSDIR)/etc/fonts/conf.d/66-vlgothic-fonts.conf; \
	else \
		$(ROMFSINST) 66-vlgothic-fonts.conf \
			/etc/fonts/conf.d/66-vlgothic-fonts.conf; \
	fi
