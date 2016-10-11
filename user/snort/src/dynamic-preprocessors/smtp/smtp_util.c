/*
 * smtp_util.c
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
 * Description:
 *
 * This file contains SMTP helper functions.
 *
 * Entry point functions:
 *
 *    safe_strchr()
 *    safe_strstr()
 *    copy_to_space()
 *    safe_sscanf()
 *
 *
 */

#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "snort_smtp.h"
#include "smtp_util.h"


/*
 * Search for a character within a buffer, safely
 *
 * @param   buf         buffer to search
 * @param   c           character to search for
 * @param   len         length of buffer to search
 *
 * @return  p           pointer to first character found
 * @retval  NULL        if character not found
 */
char * safe_strchr(char *buf, char c, u_int len)
{
    char *p = buf;
    u_int i = 0;

    while ( i < len )
    {
        if ( *p == c )
        {
            return p;
        }
        i++;
        p++;
    }

    return NULL;
}

/*
 * Search for a string within a buffer, safely
 *
 * @param   buf         buffer to search
 * @param   str         string to search for
 * @param   str_len     length of string
 * @param   buf_len     length of buffer to search
 *
 * @return  p           pointer to first character found
 * @retval  NULL        if character not found
 *
 * @note    this could be more efficient, but the search buffer should be pretty short
 */
char * safe_strstr(char *buf, char *str, u_int str_len, u_int buf_len)
{
    char *p = buf;
    u_int i = 0;

    while ( i < buf_len )
    {
        if ( memcmp(p, str, str_len) == 0 )
        {
            return p;
        }
        i++;
        p++;
    }

    return NULL;
}


/*
 * Copy up to a space char, or to buffer size
 *
 * @param   to      buffer to copy to
 * @param   from    buffer to copy from
 * @param   to_len  size of to buffer
 *
 * @return none
 */
void copy_to_space(char *to, char *from, int to_len)
{
    int i = 0;

    while ( !isspace(*from) && !isspace(*from) && i < (to_len-1) )
    {
        *to = *from;
        to++;
        from++;
        i++;
    }
    *to = '\0';
}

/*
 * Extract a number from a string
 *
 * @param   buf         buffer parse
 * @param   buf_len     max number of characters to parse
 * @param   base        base of number, e.g. 16 (hex) 10 (decimal)
 * @param   value       returned number extracted
 *
 * @return  unsigned long   value of number extracted          
 *
 * @note    this could be more efficient, but the search buffer should be pretty short
 */
u_int32_t safe_sscanf(char *buf, u_int buf_len, u_int base)
{
    char       *p = buf;
    u_int       i = 0;
    char        c = *p;
    u_int32_t   value = 0;
    
    while ( i < buf_len )
    {
        c = toupper(c);

        /* Make sure it is a number, if not return with what we have */
        if ( !(isdigit(c) || (c >= 'A' && c <= 'F')) )
            return value;

        if ( isdigit(c) )
        {
            c = c - '0';
        }
        else
        {
            c = c - 'A' + 10;
        }

        value = value*base + c;

        c = *(++p);
    }

    return value;
}

