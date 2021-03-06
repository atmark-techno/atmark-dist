/*
 * $Id: dict.h,v 1.1 2003/09/11 22:02:02 janakj Exp $
 *
 * Group Membership - Radius
 * Definitions not found in radiusclient.h
 *
 * Copyright (C) 2001-2003 Fhg Fokus
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 * -------
 * 2003-03-09: Based on ser_radius.h from radius_auth (janakj)
 */

/*
 * WARNING: Don't forget to update the dictionary if you update this file !!!
 */

#ifndef DICT_H
#define DICT_H

/* Service-Type */
#define PW_GROUP_CHECK                  12

#define PW_SIP_GROUP                    211     /* string */

#endif /* DICT_H */
