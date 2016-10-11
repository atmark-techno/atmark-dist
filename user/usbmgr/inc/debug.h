/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: debug.h,v 3.1 2000/12/01 11:16:43 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#ifndef	__USBMGR_DEBUG_H
#define	__USBMGR_DEBUG_H

#ifdef	DEBUG_ALLOC
#define		Malloc(xxx)	dmalloc((xxx))
#define		Free(xxx)	dfree((xxx))
#else
#define		Malloc(xxx)	malloc((xxx))
#define		Free(xxx)	free((xxx))
#endif

#endif	/* __USBMGR_DEBUG_H */
