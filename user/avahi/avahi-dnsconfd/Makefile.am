# $Id: Makefile.am 336 2005-08-15 21:31:30Z lennart $
#
# This file is part of avahi.
# 
# avahi is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# avahi is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
# License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with avahi; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA.

if HAVE_EXPAT
if HAVE_LIBDAEMON

pkgsysconfdir=$(sysconfdir)/avahi

AM_CFLAGS= \
	-I$(top_srcdir) \
	-DAVAHI_RUNTIME_DIR=\"$(avahi_runtime_dir)/\" \
	-DAVAHI_SOCKET=\"$(avahi_socket)\" \
	-DAVAHI_DNSCONF_SCRIPT=\"$(pkgsysconfdir)/avahi-dnsconfd.action\"

# This cool debug trap works on i386/gcc only
AM_CFLAGS+='-DDEBUG_TRAP=__asm__("int $$3")'

sbin_PROGRAMS = avahi-dnsconfd

avahi_dnsconfd_SOURCES = main.c 
avahi_dnsconfd_CFLAGS = $(AM_CFLAGS) $(LIBDAEMON_CFLAGS)
avahi_dnsconfd_LDADD = $(AM_LDADD) ../avahi-common/libavahi-common.la $(LIBDAEMON_LIBS)

pkgsysconf_SCRIPTS=avahi-dnsconfd.action

endif
endif

EXTRA_DIST=avahi-dnsconfd.action
