#ifndef SELINUX_H
#define SELINUX_H

#  ifdef WITH_SELINUX

extern void setup_selinux_pty(const char *, const char *);
extern void setup_selinux_exec_context(const char *);

#  else

static inline void setup_selinux_pty(const char *name, const char *tty) {}
static inline void setup_selinux_exec_context(const char *name) {} 

#endif /* WITH_SELINUX */
#endif /* SELINUX_H */
