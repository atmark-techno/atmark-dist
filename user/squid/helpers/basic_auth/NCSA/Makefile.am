#
#  Makefile for the Squid Object Cache server
#
#  $Id: Makefile.am,v 1.1.2.2 2002/08/12 01:51:02 squidadm Exp $
#
#  Uncomment and customize the following to suit your needs:
#

libexec_PROGRAMS	= ncsa_auth
ncsa_auth_SOURCES	= ncsa_auth.c
LDADD			= -L$(top_builddir)/lib -lmiscutil $(CRYPTLIB) $(XTRA_LIBS)
INCLUDES      = -I$(top_srcdir)/include
