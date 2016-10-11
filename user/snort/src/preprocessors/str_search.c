
/*
 * Copyright (C) 2005 Sourcefire,Inc.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>

#include "str_search.h"
#include "mpse.h"

typedef struct tag_search
{
    void *mpse;
    unsigned int max_len;
} t_search;

static t_search *_mpse = NULL;
static unsigned int  _num_mpse;

int SearchInit(unsigned int num)
{
    unsigned int i;

    _num_mpse = num;

    _mpse = malloc(sizeof(t_search) * num);
    if ( _mpse == NULL )
        return -1;

    for ( i = 0; i < num; i++ )
    {
        _mpse[i].mpse = mpseNew(MPSE_AC);
        if ( !_mpse[i].mpse )
            return -1;
        _mpse[i].max_len = 0;
    }
    return 0;
}

int SearchReInit(unsigned int i)
{
    if ( _mpse[i].mpse != NULL )
        mpseFree(_mpse[i].mpse);
    _mpse[i].mpse = mpseNew(MPSE_AC);
    _mpse[i].max_len = 0;
    
    if ( !_mpse[i].mpse )
        return -1;

    return 0;
}


void SearchFree()
{
    unsigned int i;

    if ( _mpse != NULL )
    {
        for ( i = 0; i < _num_mpse; i++ )
        {
            if ( _mpse[i].mpse != NULL )
                mpseFree(_mpse[i].mpse);
        }
        free(_mpse);
    }
}


/*  
    Do efficient search of data 
    @param   mpse_id    specify which engine to use to search
    @param   str        string to search
    @param   str_len    length of string to search
    @param   confine   1 means only search at beginning of string (confine to length of max search string)
    @param   Match      function callback when string match found
 */
int SearchFindString(unsigned int mpse_id, char *str, unsigned int str_len, int confine, int (*Match) (void *, int, void *))
{
    int num;

    if ( confine && _mpse[mpse_id].max_len != 0 )
    {
        if ( _mpse[mpse_id].max_len < str_len )
        {
            str_len = _mpse[mpse_id].max_len;
        }
    }
    num = mpseSearch(_mpse[mpse_id].mpse, str, str_len, Match, (void *) str);
    
    return num;
}


void SearchAdd(unsigned int mpse_id, char *pat, unsigned int pat_len, int id)
{
    mpseAddPattern(_mpse[mpse_id].mpse, pat, pat_len, 1, 0, 0, (void *)(long) id, 0);

    if ( pat_len > _mpse[mpse_id].max_len )
        _mpse[mpse_id].max_len = pat_len;
}

void SearchPrepPatterns(unsigned int mpse_id)
{
    mpsePrepPatterns(_mpse[mpse_id].mpse);
}

/* API exported by this module */
SearchAPI searchAPI =
{
    SearchInit,
    SearchReInit,
    SearchFree,
    SearchAdd,
    SearchPrepPatterns,
    SearchFindString
};

SearchAPI *search_api = &searchAPI;

