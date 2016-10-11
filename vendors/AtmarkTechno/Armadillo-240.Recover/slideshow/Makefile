include $(CONFIG_CONFIG)

EXEC = slideshow

SRCS = slideshow.c
OBJS = $(SRCS:.c=.o)

CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/include -I$(ROOTDIR)/user/imagemagick -I$(ROOTDIR)/lib/libjpeg
LDFLGS += -L$(ROOTDIR)/user/imagemagick/wand -L$(ROOTDIR)/user/imagemagick/magick -L$(ROOTDIR)/lib/libpng -L$(ROOTDIR)/lib/libjpeg
LDLIBS += -lWand -lMagick -lz -lpng -ljpeg -lpthread

ifneq ($(strip $(CONFIG_USER_IMAGEMAGICK)), y)
	error := imagemagick-error
endif
ifneq ($(strip $(CONFIG_LIB_LIBPNG_FORCE)), y)
	error := libpng-error
endif
ifneq ($(strip $(CONFIG_LIB_LIBJPEG_FORCE)), y)
	error := libjpeg-error
endif
ifneq ($(strip $(CONFIG_LIB_ZLIB_FORCE)), y)
	error := zlib-error
endif
ifneq ($(strip $(CONFIG_USER_UDEV_UDEVD)), y)
	error := udevd-error
endif



all: $(error) $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLGS) -o $@ $(OBJS) $(LDLIBS)

imagemagick-error:
	@echo "imagemagick is not chosen"; false

libpng-error:
	@echo "libpng is not chosen"; false

libjpeg-error:
	@echo "libjpg is not chosen"; false

zlib-error:
	@echo "zlib is not chosen"; false

udevd-error:
	@echo "udevd is not chosen"; false

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o *~

