
/*
 *  Copyright (C) 2005 Sourcefire,Inc.
 */
#ifndef __STR_SEARCH_H__
#define __STR_SEARCH_H__

/* Function prototypes  */
int  SearchInit(unsigned int num);
int  SearchReInit(unsigned int i);
void SearchFree();
void SearchAdd(unsigned int mpse_id, char *pat, unsigned int pat_len, int id);
void SearchPrepPatterns(unsigned int mpse_id);
typedef int (*MatchFunction)(void *, int, void *);
int  SearchFindString(unsigned int mpse_id, char *str, unsigned int str_len, int confine, int (*Match) (void *, int, void *));

typedef struct _search_api
{
    int (*search_init)(unsigned int);

    int (*search_reinit)(unsigned int);

    void (*search_free)();

    void (*search_add)(unsigned int, char *, unsigned int, int);

    void (*search_prep)(unsigned int);

    int (*search_find)(unsigned int, char *, unsigned int, int, MatchFunction); 
} SearchAPI;

extern SearchAPI *search_api;

#endif  /*  __STR_SEARCH_H__  */

