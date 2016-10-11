
/*
 * smtp_util.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2005 Sourcefire Inc.
 *
 * Author: Andy  Mullican
 *
 */

#ifndef __SMTP_UTIL_H__
#define __SMTP_UTIL_H__

char *    safe_strchr(char *buf, char c, u_int len);
char *    safe_strstr(char *buf, char *str, u_int str_len, u_int buf_len);
void      copy_to_space(char *to, char *from, int to_len);
u_int32_t safe_sscanf(char *buf, u_int buf_len, u_int base);

#endif  /*  __SMTP_UTIL_H__  */
