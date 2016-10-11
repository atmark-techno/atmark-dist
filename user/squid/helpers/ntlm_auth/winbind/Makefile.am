#
#  Makefile for the Squid Object Cache server
#
#  $Id: Makefile.am,v 1.1.2.2 2003/02/12 03:21:01 squidadm Exp $
#

libexec_PROGRAMS = wb_ntlmauth
wb_ntlmauth_SOURCES = wb_ntlm_auth.c wb_common.c wbntlm.h
EXTRA_DIST = \
	patches/wb_common.patch \
	patches/winbind_nss_config.patch
INCLUDES = -I. -I$(top_builddir)/include -I$(top_srcdir)/include \
    -I$(top_srcdir)/src -I@SAMBASOURCES@
LDADD = -L$(top_builddir)/lib -lmiscutil -lntlmauth $(XTRA_LIBS)
