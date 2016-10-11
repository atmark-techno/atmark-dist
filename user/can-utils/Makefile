#  Copyright (c) 2002-2005 Volkswagen Group Electronic Research
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions, the following disclaimer and
#     the referenced file 'COPYING'.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of Volkswagen nor the names of its contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
#
#  Alternatively, provided that this notice is retained in full, this
#  software may be distributed under the terms of the GNU General
#  Public License ("GPL") version 2 as distributed in the 'COPYING'
#  file from the main directory of the linux kernel source.
#
#  The provided data structures and external interfaces from this code
#  are not restricted to be used by modules with a GPL compatible license.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
#  DAMAGE.
#
#  Send feedback to <socketcan-users@lists.berlios.de>

ALL = cansend candump cangen

CFLAGS += -DSO_RXQ_OVFL=40 -DPF_CAN=29 -DAF_CAN=PF_CAN

target_y := 
target_$(CONFIG_USER_CANUTILS_CANSEND) += cansend
target_$(CONFIG_USER_CANUTILS_CANDUMP) += candump
target_$(CONFIG_USER_CANUTILS_CANGEN)  += cangen

all: $(target_y)

cansend: cansend.o lib.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

candump: candump.o lib.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

cangen: cangen.o lib.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(ALL) *.o *~

distclean:
	rm -f $(ALL) *.o *~

romfs: $(target_y)
	$(ROMFSINST) -e CONFIG_USER_CANUTILS_CANSEND /usr/bin/cansend
	$(ROMFSINST) -e CONFIG_USER_CANUTILS_CANDUMP /usr/bin/candump
	$(ROMFSINST) -e CONFIG_USER_CANUTILS_CANGEN /usr/bin/cangen

