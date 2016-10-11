#ifndef __EXP_WIN_H
#define __EXP_WIN_H
/* exp_win.h - window support

Written by: Don Libes, NIST, 10/25/93

This file is in the public domain.  However, the author and NIST
would appreciate credit if you use this file or parts of it.
*/

EXTERN int exp_window_size_set _ANSI_ARGS_((int fd));
EXTERN int exp_window_size_get _ANSI_ARGS_((int fd));

EXTERN void exp_win_rows_set _ANSI_ARGS_((char *rows));
EXTERN void exp_win_rows_get _ANSI_ARGS_((char *rows));
EXTERN void exp_win_columns_set _ANSI_ARGS_((char *columns));
EXTERN void exp_win_columns_get _ANSI_ARGS_((char *columns));

EXTERN void exp_win2_rows_set _ANSI_ARGS_((int fd, char *rows));
EXTERN void exp_win2_rows_get _ANSI_ARGS_((int fd, char *rows));
EXTERN void exp_win2_columns_set _ANSI_ARGS_((int fd, char *columns));
EXTERN void exp_win2_columns_get _ANSI_ARGS_((int fd, char *columns));

#endif /* __EXP_WIN_H */
