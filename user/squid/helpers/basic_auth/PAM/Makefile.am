#
#  Makefile for the Squid PAM authentication helper
#
#  $Id: Makefile.am,v 1.1.2.2 2002/08/12 01:51:02 squidadm Exp $
#
#  Uncomment and customize the following to suit your needs:
#

INCLUDES	= -I$(top_srcdir)/include

man_MANS		= pam_auth.8
EXTRA_DIST		= pam_auth.8
libexec_PROGRAMS	= pam_auth
LDADD			= -L$(top_builddir)/lib -lmiscutil -lpam $(XTRA_LIBS)
