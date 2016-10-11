#
#  Makefile for the Squid Object Cache server
#
#  $Id: Makefile.am,v 1.1.2.1 2002/06/28 14:46:21 squidadm Exp $
#

SUBDIRS = smbval

libexec_PROGRAMS = ntlm_auth
ntlm_auth_SOURCES = libntlmssp.c ntlm_auth.c ntlm.h
INCLUDES      = -I. -I$(top_builddir)/include -I$(top_srcdir)/include \
		-I$(top_srcdir)/src/ -I$(srcdir)/smbval
LDADD		= smbval/libsmbvalid.a -L$(top_builddir)/lib -lntlmauth \
		  -lmiscutil $(CRYPTLIB) $(XTRA_LIBS)
