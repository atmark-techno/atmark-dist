#
#	Makefile -- Takao Fonts
#

-include $(CONFIG_CONFIG)

empty =
space = $(empty) $(empty)
escape = $(empty)\$(empty)
fakespace = @

file_y = IPA_Font_License_Agreement_v1.0.txt
file_$(CONFIG_FONTS_TAKAO_GOTHIC) += TakaoGothic.ttf
file_$(CONFIG_FONTS_TAKAO_PGOTHIC) += TakaoPGothic.ttf
file_$(CONFIG_FONTS_TAKAO_EXGOTHIC) += TakaoExGothic.ttf
file_$(CONFIG_FONTS_TAKAO_MINCHO) += TakaoMincho.ttf
file_$(CONFIG_FONTS_TAKAO_PMINCHO) += TakaoPMincho.ttf
file_$(CONFIG_FONTS_TAKAO_EXMINCHO) += TakaoExMincho.ttf

family_$(CONFIG_FONTS_TAKAO_GOTHIC) += Takaoゴシック TakaoGothic
family_$(CONFIG_FONTS_TAKAO_PGOTHIC) += Takao$(fakespace)Pゴシック TakaoPGothic
family_$(CONFIG_FONTS_TAKAO_EXGOTHIC) += TakaoExゴシック TakaoExGothic
family_$(CONFIG_FONTS_TAKAO_MINCHO) += Takao明朝 TakaoMincho
family_$(CONFIG_FONTS_TAKAO_PMINCHO) += Takao$(fakespace)P明朝 TakaoPMincho
family_$(CONFIG_FONTS_TAKAO_EXMINCHO) += TakaoEx明朝 TakaoExMincho

all:
# nothing to do here

romfs:
	mkdir -p $(ROMFSDIR)/usr/share/fonts/truetype/fonts-takao/
	for i in $(file_y); do \
		$(ROMFSINST) $$i /usr/share/fonts/truetype/fonts-takao/$$i; \
	done

	mkdir -p $(ROMFSDIR)/etc/fonts/conf.d/
	if [ "$(family_)" ]; then \
		cat 65-takao-fonts.conf | \
		grep -v $(subst $(fakespace),$(escape)$(space),$(addprefix -e ,$(family_))) > $(ROMFSDIR)/etc/fonts/conf.d/65-takao-fonts.conf; \
	else \
		$(ROMFSINST) 65-takao-fonts.conf \
			/etc/fonts/conf.d/65-takao-fonts.conf; \
	fi
