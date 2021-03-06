/* $Id: f_malloc.c,v 1.12 2003/08/15 14:19:18 andrei Exp $
 *
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
 */
/*
 * History:
 * --------
 *              created by andrei
 *  2003-07-06  added fm_realloc (andrei)
 */


#if !defined(q_malloc) && !(defined VQ_MALLOC)  && (defined F_MALLOC)

#include <string.h>
#include <stdlib.h>

#include "f_malloc.h"
#include "../dprint.h"
#include "../globals.h"


/*useful macros*/

#define FRAG_NEXT(f) \
	((struct fm_frag*)((char*)(f)+sizeof(struct fm_frag)+(f)->size ))

#define FRAG_OVERHEAD	(sizeof(struct fm_frag))


/* ROUNDTO= 2^k so the following works */
#define ROUNDTO_MASK	(~((unsigned long)ROUNDTO-1))
#define ROUNDUP(s)		(((s)+(ROUNDTO-1))&ROUNDTO_MASK)
#define ROUNDDOWN(s)	((s)&ROUNDTO_MASK)

/*
 #define ROUNDUP(s)		(((s)%ROUNDTO)?((s)+ROUNDTO)/ROUNDTO*ROUNDTO:(s))
 #define ROUNDDOWN(s)	(((s)%ROUNDTO)?((s)-ROUNDTO)/ROUNDTO*ROUNDTO:(s))
*/



	/* finds the hash value for s, s=ROUNDTO multiple*/
#define GET_HASH(s)   ( ((s)<F_MALLOC_OPTIMIZE)?(s)/ROUNDTO: \
						F_MALLOC_OPTIMIZE/ROUNDTO+big_hash_idx((s))- \
							F_MALLOC_OPTIMIZE_FACTOR+1 )

#define UN_HASH(h)	( ((h)<(F_MALLOC_OPTIMIZE/ROUNDTO))?(h)*ROUNDTO: \
						1<<((h)-F_MALLOC_OPTIMIZE/ROUNDTO+\
							F_MALLOC_OPTIMIZE_FACTOR-1)\
					)


/* computes hash number for big buckets*/
inline static int big_hash_idx(int s)
{
	int idx;
	/* s is rounded => s = k*2^n (ROUNDTO=2^n) 
	 * index= i such that 2^i > s >= 2^(i-1)
	 *
	 * => index = number of the first non null bit in s*/
	for (idx=31; !(s&0x80000000) ; s<<=1, idx--);
	return idx;
}


#ifdef DBG_F_MALLOC
#define ST_CHECK_PATTERN   0xf0f0f0f0
#define END_CHECK_PATTERN1 0xc0c0c0c0
#define END_CHECK_PATTERN2 0xabcdefed
#endif



static inline void fm_insert_free(struct fm_block* qm, struct fm_frag* frag)
{
	struct fm_frag** f;
	int hash;
	
	hash=GET_HASH(frag->size);
	f=&(qm->free_hash[hash]);
	if (frag->size > F_MALLOC_OPTIMIZE){
		for(; *f; f=&((*f)->u.nxt_free)){
			if (frag->size <= (*f)->size) break;
		}
	}
	
	/*insert it here*/
	frag->u.nxt_free=*f;
	*f=frag;
}



 /* size should be already rounded-up */
static inline
#ifdef DBG_F_MALLOC 
void fm_split_frag(struct fm_block* qm, struct fm_frag* frag,unsigned int size,
					char* file, char* func, unsigned int line)
#else
void fm_split_frag(struct fm_block* qm, struct fm_frag* frag,unsigned int size)
#endif
{
	unsigned int rest;
	struct fm_frag* n;
	
	rest=frag->size-size;
	if (rest>(FRAG_OVERHEAD+MIN_FRAG_SIZE)){
		frag->size=size;
		/*split the fragment*/
		n=FRAG_NEXT(frag);
		n->size=rest-FRAG_OVERHEAD;
#ifdef DBG_F_MALLOC
		qm->real_used+=FRAG_OVERHEAD;
		/* frag created by malloc, mark it*/
		n->file=file;
		n->func="frag. from fm_malloc";
		n->line=line;
		n->check=ST_CHECK_PATTERN;
#endif
		/* reinsert n in free list*/
		fm_insert_free(qm, n);
	}else{
		/* we cannot split this fragment any more => alloc all of it*/
	}
}



/* init malloc and return a fm_block*/
struct fm_block* fm_malloc_init(char* address, unsigned int size)
{
	char* start;
	char* end;
	struct fm_block* qm;
	unsigned int init_overhead;
	
	/* make address and size multiple of 8*/
	start=(char*)ROUNDUP((unsigned long) address);
	if (size<start-address) return 0;
	size-=(start-address);
	if (size <(MIN_FRAG_SIZE+FRAG_OVERHEAD)) return 0;
	size=ROUNDDOWN(size);

	init_overhead=(ROUNDUP(sizeof(struct fm_block))+sizeof(struct fm_frag));
	
	
	if (size < init_overhead)
	{
		/* not enough mem to create our control structures !!!*/
		return 0;
	}
	end=start+size;
	qm=(struct fm_block*)start;
	memset(qm, 0, sizeof(struct fm_block));
	size-=init_overhead;
	qm->size=size;
#ifdef DBG_F_MALLOC
	qm->real_used=init_overhead;
	qm->max_real_used=qm->real_used;
#endif
	
	qm->first_frag=(struct fm_frag*)(start+ROUNDUP(sizeof(struct fm_block)));
	qm->last_frag=(struct fm_frag*)(end-sizeof(struct fm_frag));
	/* init initial fragment*/
	qm->first_frag->size=size;
	qm->last_frag->size=0;
	
#ifdef DBG_F_MALLOC
	qm->first_frag->check=ST_CHECK_PATTERN;
	qm->last_frag->check=END_CHECK_PATTERN1;
#endif
	
	/* link initial fragment into the free list*/
	
	fm_insert_free(qm, qm->first_frag);
	
	
	return qm;
}



#ifdef DBG_F_MALLOC
void* fm_malloc(struct fm_block* qm, unsigned int size, char* file, char* func,
					unsigned int line)
#else
void* fm_malloc(struct fm_block* qm, unsigned int size)
#endif
{
	struct fm_frag** f;
	struct fm_frag* frag;
	int hash;
	
#ifdef DBG_F_MALLOC
	DBG("fm_malloc(%p, %d) called from %s: %s(%d)\n", qm, size, file, func,
			line);
#endif
	/*size must be a multiple of 8*/
	size=ROUNDUP(size);
/*	if (size>(qm->size-qm->real_used)) return 0; */

	
	/*search for a suitable free frag*/

	for(hash=GET_HASH(size);hash<F_HASH_SIZE;hash++){
		f=&(qm->free_hash[hash]);
		for(;(*f); f=&((*f)->u.nxt_free))
			if ((*f)->size>=size) goto found;
		/* try in a bigger bucket */
	}
	/* not found, bad! */
	return 0;

found:
	/* we found it!*/
	/* detach it from the free list*/
	frag=*f;
	*f=frag->u.nxt_free;
	frag->u.nxt_free=0; /* mark it as 'taken' */
	
	/*see if we'll use full frag, or we'll split it in 2*/
	
#ifdef DBG_F_MALLOC
	fm_split_frag(qm, frag, size, file, func, line);
	qm->real_used+=frag->size;
	qm->used+=frag->size;

	if (qm->max_real_used<qm->real_used)
		qm->max_real_used=qm->real_used;

	frag->file=file;
	frag->func=func;
	frag->line=line;
	frag->check=ST_CHECK_PATTERN;
	DBG("fm_malloc(%p, %d) returns address %p \n", qm, size,
		(char*)frag+sizeof(struct fm_frag));
#else
	fm_split_frag(qm, frag, size);
#endif
	return (char*)frag+sizeof(struct fm_frag);
}



#ifdef DBG_F_MALLOC
void fm_free(struct fm_block* qm, void* p, char* file, char* func, 
				unsigned int line)
#else
void fm_free(struct fm_block* qm, void* p)
#endif
{
	struct fm_frag* f;
	unsigned int size;

#ifdef DBG_F_MALLOC
	DBG("fm_free(%p, %p), called from %s: %s(%d)\n", qm, p, file, func, line);
	if (p>(void*)qm->last_frag || p<(void*)qm->first_frag){
		LOG(L_CRIT, "BUG: fm_free: bad pointer %p (out of memory block!) - "
				"aborting\n", p);
		abort();
	}
#endif
	if (p==0) {
		LOG(L_WARN, "WARNING:fm_free: free(0) called\n");
		return;
	}
	f=(struct fm_frag*) ((char*)p-sizeof(struct fm_frag));
#ifdef DBG_F_MALLOC
	DBG("fm_free: freeing block alloc'ed from %s: %s(%ld)\n", f->file, f->func,
			f->line);
#endif
	size=f->size;

#ifdef DBG_F_MALLOC
	qm->used-=size;
	qm->real_used-=size;
	f->file=file;
	f->func=func;
	f->line=line;
#endif
	fm_insert_free(qm, f);
}


#ifdef DBG_F_MALLOC
void* fm_realloc(struct fm_block* qm, void* p, unsigned int size,
					char* file, char* func, unsigned int line)
#else
void* fm_realloc(struct fm_block* qm, void* p, unsigned int size)
#endif
{
	struct fm_frag *f;
	struct fm_frag **pf;
	unsigned int diff;
	unsigned int orig_size;
	struct fm_frag *n;
	void *ptr;
	
#ifdef DBG_F_MALLOC
	DBG("fm_realloc(%p, %p, %d) called from %s: %s(%d)\n", qm, p, size,
			file, func, line);
	if ((p)&&(p>(void*)qm->last_frag || p<(void*)qm->first_frag)){
		LOG(L_CRIT, "BUG: fm_free: bad pointer %p (out of memory block!) - "
				"aborting\n", p);
		abort();
	}
#endif
	if (size==0) {
		if (p)
#ifdef DBG_F_MALLOC
			fm_free(qm, p, file, func, line);
#else
			fm_free(qm, p);
#endif
		return 0;
	}
	if (p==0)
#ifdef DBG_F_MALLOC
		return fm_malloc(qm, size, file, func, line);
#else
		return fm_malloc(qm, size);
#endif
	f=(struct fm_frag*) ((char*)p-sizeof(struct fm_frag));
#ifdef DBG_F_MALLOC
	DBG("fm_realloc: realloc'ing frag %p alloc'ed from %s: %s(%ld)\n",
			f, f->file, f->func, f->line);
#endif
	size=ROUNDUP(size);
	orig_size=f->size;
	if (f->size > size){
		/* shrink */
#ifdef DBG_F_MALLOC
		DBG("fm_realloc: shrinking from %ld to %d\n", f->size, size);
		fm_split_frag(qm, f, size, file, "frag. from fm_realloc", line);
		qm->real_used-=(orig_size-f->size);
		qm->used-=(orig_size-f->size);
#else
		fm_split_frag(qm, f, size);
#endif
	}else if (f->size<size){
		/* grow */
#ifdef DBG_F_MALLOC
		DBG("fm_realloc: growing from %ld to %d\n", f->size, size);
#endif
		diff=size-f->size;
		n=FRAG_NEXT(f);
		if (((char*)n < (char*)qm->last_frag) && 
				(n->u.nxt_free)&&((n->size+FRAG_OVERHEAD)>=diff)){
			/* join  */
			/* detach n from the free list */
			pf=&(qm->free_hash[GET_HASH(n->size)]);
			/* find it */
			for(;(*pf)&&(*pf!=n); pf=&((*pf)->u.nxt_free));
			if (*pf==0){
				/* not found, bad! */
				LOG(L_CRIT, "BUG: fm_realloc: could not find %p in free "
						"list (hash=%ld)\n", n, GET_HASH(n->size));
				abort();
			}
			/* detach */
			*pf=n->u.nxt_free;
			/* join */
			f->size+=n->size+FRAG_OVERHEAD;
		#ifdef DBG_F_MALLOC
			qm->real_used-=FRAG_OVERHEAD;
		#endif
			/* split it if necessary */
			if (f->size > size){
		#ifdef DBG_F_MALLOC
				fm_split_frag(qm, f, size, file, "fragm. from fm_realloc",
						line);
		#else
				fm_split_frag(qm, f, size);
		#endif
			}
		#ifdef DBG_F_MALLOC
			qm->real_used+=(f->size-orig_size);
			qm->used+=(f->size-orig_size);
		#endif
		}else{
			/* could not join => realloc */
	#ifdef DBG_F_MALLOC
			ptr=fm_malloc(qm, size, file, func, line);
	#else
			ptr=fm_malloc(qm, size);
	#endif
			if (ptr)
				/* copy, need by libssl */
				memcpy(ptr, p, orig_size);
	#ifdef DBG_F_MALLOC
				fm_free(qm, p, file, func, line);
	#else
				fm_free(qm, p);
	#endif
				p=ptr;
			}
	}else{
		/* do nothing */
#ifdef DBG_F_MALLOC
		DBG("fm_realloc: doing nothing, same size: %ld - %d\n", f->size, size);
#endif
	}
#ifdef DBG_F_MALLOC
	DBG("fm_realloc: returning %p\n", p);
#endif
	return p;
}



void fm_status(struct fm_block* qm)
{
	struct fm_frag* f;
	int i,j;
	int h;
	long size;

	LOG(memlog, "fm_status (%p):\n", qm);
	if (!qm) return;

	LOG(memlog, " heap size= %ld\n", qm->size);
#ifdef DBG_F_MALLOC
	LOG(memlog, " used= %ld, used+overhead=%ld, free=%ld\n",
			qm->used, qm->real_used, qm->size-qm->real_used);
	LOG(memlog, " max used (+overhead)= %ld\n", qm->max_real_used);
#endif
	/*
	LOG(memlog, "dumping all fragments:\n");
	for (f=qm->first_frag, i=0;((char*)f<(char*)qm->last_frag) && (i<10);
			f=FRAG_NEXT(f), i++){
		LOG(memlog, "    %3d. %c  address=%x  size=%d\n", i, 
				(f->u.reserved)?'a':'N',
				(char*)f+sizeof(struct fm_frag), f->size);
#ifdef DBG_F_MALLOC
		LOG(memlog, "            %s from %s: %s(%d)\n",
				(f->u.is_free)?"freed":"alloc'd", f->file, f->func, f->line);
#endif
	}
*/
	LOG(memlog, "dumping free list:\n");
	for(h=0,i=0,size=0;h<F_HASH_SIZE;h++){
		
		for (f=qm->free_hash[h],j=0; f; size+=f->size,f=f->u.nxt_free,i++,j++);
		if (j) LOG(memlog, "hash = %3d fragments no.: %5d,\n\t\t"
							" bucket size: %9ld - %9ld (first %9ld)\n",
							h, j, (long)UN_HASH(h),
						(long)((h<F_MALLOC_OPTIMIZE/ROUNDTO)?1:2)*UN_HASH(h),
							qm->free_hash[h]->size
				);
		/*
		{
			LOG(memlog, "   %5d.[%3d:%3d] %c  address=%x  size=%d(%x)\n",
					i, h, j,
					(f->u.reserved)?'a':'N',
					(char*)f+sizeof(struct fm_frag), f->size, f->size);
#ifdef DBG_F_MALLOC
			DBG("            %s from %s: %s(%d)\n", 
				(f->u.reserved)?"freed":"alloc'd", f->file, f->func, f->line);
#endif
		}
	*/
	}
	LOG(memlog, "TOTAL: %6d free fragments = %6ld free bytes\n", i, size);
	LOG(memlog, "-----------------------------\n");
}




#endif
