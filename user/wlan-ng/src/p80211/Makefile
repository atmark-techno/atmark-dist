# src/p80211/Makefile 
#
# Copyright (C) 1999 AbsoluteValue Systems, Inc.  All Rights Reserved.
# --------------------------------------------------------------------
#
# linux-wlan
#
#   The contents of this file are subject to the Mozilla Public
#   License Version 1.1 (the "License"); you may not use this file
#   except in compliance with the License. You may obtain a copy of
#   the License at http://www.mozilla.org/MPL/
#
#   Software distributed under the License is distributed on an "AS
#   IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
#   implied. See the License for the specific language governing
#   rights and limitations under the License.
#
#   Alternatively, the contents of this file may be used under the
#   terms of the GNU Public License version 2 (the "GPL"), in which
#   case the provisions of the GPL are applicable instead of the
#   above.  If you wish to allow the use of your version of this file
#   only under the terms of the GPL and not to allow others to use
#   your version of this file under the MPL, indicate your decision
#   by deleting the provisions above and replace them with the notice
#   and other provisions required by the GPL.  If you do not delete
#   the provisions above, a recipient may use your version of this
#   file under either the MPL or the GPL.
#
# --------------------------------------------------------------------
#
# Inquiries regarding the linux-wlan Open Source project can be
# made directly to:
#
# AbsoluteValue Systems Inc.
# info@linux-wlan.com
# http://www.linux-wlan.com
#
# --------------------------------------------------------------------
#
# Portions of the development of this software were funded by 
# Intersil Corporation as part of PRISM(R) chipset product development.
#
# --------------------------------------------------------------------

-include $(WLAN_SRC)/../config.mk

ifeq ($(WLAN_DEBUG), y)
WLAN_INCLUDE_DEBUG=-DWLAN_INCLUDE_DEBUG
endif

ifneq ($(KERNELRELEASE),)

EXTRA_CFLAGS += -I$(WLAN_SRC)/include $(WLAN_INCLUDE_DEBUG)

ifeq ($(KERN_25), y)

p80211-objs := p80211mod.o p80211conv.o p80211req.o p80211frm.o p80211wep.o p80211wext.o p80211netdev.o
obj-m := p80211.o

else # kbuild-2.4

O_TARGET := p80211.o
obj-m := $(O_TARGET)
obj-y := p80211mod.o p80211conv.o p80211req.o p80211frm.o p80211wep.o p80211wext.o p80211netdev.o p80211mod.o
export-objs := p80211mod.o

include $(TOPDIR)/Rules.make

endif # kbuild switch

else  # if KERNELRELEASE is not set..

PWD := $(shell pwd)
default:

ifeq ($(KERN_25), y)
	$(MAKE) -C $(LINUX_SRC) M='$(PWD)/.. $(PWD)' WLAN_SRC=$(WLAN_SRC) modules
else
	$(MAKE) -C $(LINUX_SRC) SUBDIRS=$(PWD) WLAN_SRC=$(WLAN_SRC) dep modules
endif # KERN_25

clean:
	rm -f *.o *.ko .*.cmd *.mod.c  *.flags .*.flags

install:
	mkdir -p $(TARGET_MODDIR)
ifeq ($(KERN_25), y)
	cp *.ko	$(TARGET_MODDIR)
else
	cp p80211.o	$(TARGET_MODDIR)
endif

endif # KERNRELEASE
