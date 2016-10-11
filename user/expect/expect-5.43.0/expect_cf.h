/* expect_cf.h.  Generated automatically by configure.  */
/*
 * Check for headers
 */
#ifndef __EXPECT_CF_H__
#define __EXPECT_CF_H__

/* #undef NO_STDLIB_H */		/* Tcl requires this name */
/* #undef NO_UNION_WAIT */
/* #undef HAVE_STDARG_H */
/* #undef HAVE_VARARGS_H */
#define HAVE_STROPTS_H 1
/* #undef HAVE_SYSCONF_H */
#define HAVE_SYS_FCNTL_H 1
#define HAVE_SYS_WAIT_H 1
/* #undef HAVE_SYS_BSDTYPES_H */	/* nice ISC special */
#define HAVE_SYS_SELECT_H 1	/* nice ISC special */
#define HAVE_SYS_TIME_H 1		/* nice ISC special */
/* #undef HAVE_SYS_PTEM_H */		/* SCO needs this for window size */
/* #undef HAVE_STRREDIR_H */		/* Solaris needs this for console redir */
/* #undef HAVE_STRPTY_H */		/* old-style Dynix ptys need this */
#define HAVE_UNISTD_H 1
#define HAVE_SYSMACROS_H 1
#define HAVE_INTTYPES_H 1
/* #undef HAVE_TIOCGWINSZ_IN_TERMIOS_H */
/* #undef HAVE_TCGETS_OR_TCGETA_IN_TERMIOS_H */

/* #undef pid_t */
#define RETSIGTYPE void
#define TIME_WITH_SYS_TIME 1	/* ok to include both time.h and sys/time.h */
#define SETPGRP_VOID 1		/* if setpgrp takes 0 args */

/*
 * This section is for compile macros needed by
 * everything else.
 */

/*
 * Check for functions
 */
#define HAVE_MEMCPY 1
#define HAVE_SYSCONF 1
/* #undef SIMPLE_EVENT */
#define HAVE_STRFTIME 1
#define HAVE_MEMMOVE 1
#define HAVE_TIMEZONE 1		/* timezone() a la Pyramid */
#define HAVE_SIGLONGJMP 1
#define HAVE_STRCHR 1

#ifndef HAVE_STRCHR
#define strchr(s,c) index(s,c)
#endif /* HAVE_STRCHR */

/*
 * timezone
 */
#define HAVE_SV_TIMEZONE 1

/*
 * wait status type
 */
/* #undef NO_UNION_WAIT */

/* #undef WNOHANG_REQUIRES_POSIX_SOURCE */

/*
 * Signal stuff. Setup the return type
 * and if signals need to be re-armed.
 */
/*#ifndef RETSIGTYPE*/
/*#define RETSIGTYPE void*/
/*#endif*/
/* #undef REARM_SIG */

/*
 * Generate correct type for select mask
 */
#ifndef SELECT_MASK_TYPE
#define SELECT_MASK_TYPE fd_set
#endif

/*
 * Check how stty works
 */
/* #undef STTY_READS_STDOUT */

/*
 * Check for tty/pty functions and structures
 */
#define POSIX 1
#define HAVE_TCSETATTR 1
#define HAVE_TERMIO 1
#define HAVE_TERMIOS 1
/* #undef HAVE_SGTTYB */
/* #undef HAVE__GETPTY */
/* #undef HAVE_GETPTY */
#define HAVE_OPENPTY 1
/* #undef HAVE_PTC */
/* #undef HAVE_PTC_PTS */
/* #undef HAVE_PTYM */
/* #undef HAVE_PTYTRAP */
#define HAVE_PTMX 1
/* #undef HAVE_PTMX_BSD */
/* #undef HAVE_SCO_CLIST_PTYS */

/*
 * Special hacks
 */
/* #undef CONVEX */
/* #undef SOLARIS */

#ifdef SOLARIS
#define __EXTENSIONS__
#endif /* SOLARIS */

#define WNOHANG_BACKUP_VALUE 1

#endif	/* __EXPECT_CF_H__ */
