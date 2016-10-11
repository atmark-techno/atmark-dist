#
# build a smart "symlink" header tree so the compile lines are
# much shorter and the problems with include paths are much smaller
#
# David McCullough <davidm@snapgear.com>
#

#
# we need the c++ includes as well
#
#
CPP_DIR   := $(shell echo -e '\#include <new>' > test.cc ; $(CXX) -E -c test.cc | sed -e 's?^\#.*"\(.*\)/new".*$$?\1?p' -e 'd' | head -1; rm -f test.cc )

#
# hopefully deal with broken compilers
#
ifeq ($(CPP_DIR),)
CPP_DIR = /usr/local/include/g++-v3
endif

#
# glibc contains lots of nasty headers in a bad location,  so we need
# to cheat a little here
#

ifeq ($(findstring glibc,$(LIBCDIR)),glibc)
LIBCINC := $(LIBCDIR)/build
else
LIBCINC := $(LIBCDIR)
endif

ifneq ($(wildcard $(ROOTDIR)/$(LINUXDIR)/include/generated/autoconf.h),)
GENERATEDAUTOCONF = $(ROOTDIR)/$(LINUXDIR)/include/generated/autoconf.h
else
GENERATEDAUTOCONF =
endif

LINKHDRS = \
	$(ROOTDIR)/lib/STLport/stlport/*,STLport \
	$(ROOTDIR)/lib/$(LIBCINC)/include/*,. \
	$(ROOTDIR)/lib/$(LIBCINC)/include/machine,. \
	$(ROOTDIR)/lib/libgmp/gmp.h,. \
	$(ROOTDIR)/lib/libgmp/mp.h,. \
	$(ROOTDIR)/lib/libgmp/gmp-impl.h,. \
	$(ROOTDIR)/lib/libgmp/longlong.h,. \
	$(ROOTDIR)/lib/libgmp/urandom.h,. \
	$(ROOTDIR)/lib/libm/*.h,. \
	$(ROOTDIR)/lib/libnet/*.h,. \
	$(ROOTDIR)/freeswan/libdes/*.h,. \
	$(ROOTDIR)/freeswan/libcrypto/libaes/*.h,. \
	$(ROOTDIR)/lib/libpcap/*.h,. \
	$(ROOTDIR)/lib/libpcap/net/bpf.h,net \
	$(ROOTDIR)/lib/libssl/include/*,. \
	$(ROOTDIR)/lib/zlib/*.h,. \
	$(ROOTDIR)/prop/libsnapgear/include/*.h,. \
	$(ROOTDIR)/prop/uberconfig/include/*.h,. \
	$(ROOTDIR)/prop/libsnapgear++/include/*,. \
	$(ROOTDIR)/lib/libcrypt_old/*.h,. \
	$(ROOTDIR)/lib/libldap/include/*,. \
	$(ROOTDIR)/lib/libldap/builddir/include/*,. \
	$(ROOTDIR)/lib/libupnp/upnp/inc/upnp,. \
	$(ROOTDIR)/lib/libatm/src/include/atm*.h,. \
	$(ROOTDIR)/lib/flex/build/FlexLexer.h,. \
	$(ROOTDIR)/$(LINUXDIR)/include/linux/*,linux \
	$(GENERATEDAUTOCONF),linux \
	$(ROOTDIR)/$(LINUXDIR)/include/asm-*,. \
	$(ROOTDIR)/$(LINUXDIR)/include/asm,. \
	$(ROOTDIR)/$(LINUXDIR)/include/mtd,. \
	$(ROOTDIR)/config,. \
	$(CPP_DIR)/new,c++ \
	$(CPP_DIR)/exception,c++ \
	$(CPP_DIR)/cstddef,c++ \
	$(CPP_DIR)/typeinfo,c++ \
	$(CPP_DIR)/bits/std_cstddef.h,c++/bits \
	$(ROOTDIR)/lib/expat/lib/expat.h,. \
	$(ROOTDIR)/user/tinytcl/*.h,. \
	$(ROOTDIR)/prop/config/config*.h,. \
	$(ROOTDIR)/prop/config/if_*.h,. \
	$(ROOTDIR)/prop/config/libconfig.h,. \
	$(ROOTDIR)/prop/config/mcapi.h,. \
	$(ROOTDIR)/prop/setkey/key.h,key \
	$(ROOTDIR)/prop/config/meta*.h,. \
	$(ROOTDIR)/prop/uberconfig/include/*.h,. \
	$(ROOTDIR)/prop/gccmapping/ValidateUpdate.h,. \
	$(ROOTDIR)/modules/autoconf.h,modules \
	$(ROOTDIR)/prop/smgrd/unixware/cmd/sscep/sscep_error_codes.h,.

.PHONY: all romfs clean


#
# link in the files,  avoid missing or CVS files,  existing files take
# precedance so we do not try to overwrite them
#

all: clean
	@echo "Making symlinks in include/"
	@for p in $(LINKHDRS); do \
		src=`expr "$$p" : '\(.*\),.*'` ; \
		dst=`expr "$$p" : '.*,\(.*\)'` ; \
		for i in $$src; do \
			[ ! -e "$$i" ] && continue; \
			[ "`basename $$i`" = CVS ] && continue; \
			[ -L "$$dst/`basename $$i`" ] && continue; \
			[ -d $$dst ] || mkdir $$dst; \
			ln -s $$i $$dst/.; \
		done ; \
	done

romfs:

clean:
	-find . -depth -type l -a ! -name Makefile | xargs rm > /dev/null 2>&1 || exit 0
	-find . -depth -type d | xargs rmdir > /dev/null 2>&1 || exit 0
