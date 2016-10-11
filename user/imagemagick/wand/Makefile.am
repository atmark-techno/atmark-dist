#  Copyright 1999-2005 ImageMagick Studio LLC, a non-profit organization
#  dedicated to making software imaging solutions freely available.
#
#  You may not use this file except in compliance with the License.  You may
#  obtain a copy of the License at
#
#    http://www.imagemagick.org/script/license.php
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#  Makefile for building Wand library.
#

wandincdir = $(topincludedir)/wand

WAND_SOURCES = \
  wand/drawing-wand.c \
	wand/drawing-wand.h \
	wand/magick-attribute.c \
	wand/magick-attribute.h \
	wand/magick-image.c \
	wand/magick-image.h \
	wand/magick-wand.c \
	wand/magick-wand.h \
	wand/magick-wand-private.h \
	wand/pixel-iterator.c \
	wand/pixel-iterator.h \
	wand/pixel-wand.c \
	wand/pixel-wand.h \
	wand/pixel-wand-private.h \
	wand/studio.h \
	wand/wand.c \
	wand/wand.h \
	wand/wand-config.h

WAND_INCLUDE_HDRS = \
	wand/drawing-wand.h \
	wand/magick-attribute.h \
	wand/magick-image.h \
	wand/magick-wand.h \
	wand/magick_wand.h \
	wand/pixel-iterator.h \
	wand/pixel-wand.h 

WAND_NOINST_HDRS = \
  wand/magick-wand-private.h \
  wand/pixel-wand-private.h \
  wand/studio.h \
  wand/wand.h

# Headers which are installed
wandinc_HEADERS = \
	$(WAND_INCLUDE_HDRS)

WAND_BIN_SCRPTS = \
	wand/Wand-config

WAND_PKGCONFIG = \
	wand/Wand.pc

WAND_MANS = \
	wand/Wand-config.1

WAND_BUILT_SRCS = \
  wand/wand-config.h

LIBWAND=wand/libWand.la

wand_libWand_la_SOURCES = $(WAND_SOURCES)
wand_libWand_la_LDFLAGS = -no-undefined \
	-version-info $(LIBRARY_CURRENT):$(LIBRARY_REVISION):$(LIBRARY_AGE)
wand_libWand_la_LIBADD = $(LIBMAGICK) $(LIB_MATH)

WAND_EXTRA_DIST = \
	wand/ChangeLog \
	wand/Wand-config.1 \
	wand/input_256c.miff \
	wand/input_bilevel.miff \
	wand/input_gray.miff \
	wand/input_truecolor.miff \
	wand/sequence.miff

wand_drawtest_SOURCES      = wand/drawtest.c
wand_drawtest_LDFLAGS      = $(LDFLAGS)
wand_drawtest_LDADD        = $(LIBWAND)

wand_wandtest_SOURCES      = wand/wandtest.c
wand_wandtest_LDFLAGS      = $(LDFLAGS)
wand_wandtest_LDADD        = $(LIBWAND)

WAND_CHECK_PGRMS = \
	wand/drawtest \
	wand/wandtest

WAND_CLEANFILES = \
	wand/*_out.* \
	wand/image.miff


