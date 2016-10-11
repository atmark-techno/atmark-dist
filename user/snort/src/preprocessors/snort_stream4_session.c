/* $Id$ */

/*
** Copyright (C) 2005 Sourcefire, Inc.
** AUTHOR: Steven Sturges <ssturges@sourcefire.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* snort_stream4_session.c
 * 
 * Purpose: Hash Table implementation of session management functions for
 *          TCP stream preprocessor.
 *
 * Arguments:
 *   
 * Effect:
 *
 * Comments:
 *
 * Any comments?
 *
 */

#define _STREAM4_INTERNAL_USAGE_ONLY_

#include "sfxhash.h"
#include "ubi_SplayTree.h"
#include "decode.h"
#include "debug.h"
#include "stream.h"
#include "log.h"

/* splay tree root data */
static ubi_trRoot s_cache;
static ubi_trRootPtr RootPtr = &s_cache;

/* Stuff defined in stream4.c that we use */
extern void DeleteSession(Session *, u_int32_t);
extern Stream4Data s4data;
extern u_int32_t stream4_memory_usage;

static SFXHASH *sessionHashTable = NULL;

#include "snort.h"
#include "profiler.h"
#ifdef PERF_PROFILING
extern PreprocStats stream4LUSessPerfStats;
#endif

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef USE_HASH_TABLE
int GetSessionCount()
{
    if (sessionHashTable)
        return sessionHashTable->count;
    else
        return 0;
}

int GetSessionKey(Packet *p, SessionHashKey *key)
{
    if (!key)
        return 0;

    if (p->iph->ip_src.s_addr < p->iph->ip_dst.s_addr)
    {
        key->lowIP = p->iph->ip_src.s_addr;
        key->port = p->tcph->th_sport;
        key->highIP = p->iph->ip_dst.s_addr;
        key->port2 = p->tcph->th_dport;
    }
    else if (p->iph->ip_src.s_addr == p->iph->ip_dst.s_addr)
    {
        key->lowIP = p->iph->ip_src.s_addr;
        key->highIP = p->iph->ip_dst.s_addr;
        if (p->tcph->th_sport < p->tcph->th_dport)
        {
            key->port = p->tcph->th_sport;
            key->port2 = p->tcph->th_dport;
        }
        else
        {
            key->port = p->tcph->th_sport;
            key->port2 = p->tcph->th_dport;
        }
    }
    else
    {
        key->lowIP = p->iph->ip_dst.s_addr;
        key->port = p->tcph->th_dport;
        key->highIP = p->iph->ip_src.s_addr;
        key->port2 = p->tcph->th_sport;
    }

#ifdef _LP64
    key->pad1 = key->pad2 = 0;
#endif

    return 1;
}

Session *GetSessionFromHashTable(Packet *p)
{
    Session *returned = NULL;
    SFXHASH_NODE *hnode;
    SessionHashKey sessionKey;

    if (!GetSessionKey(p, &sessionKey))
        return NULL;

    hnode = sfxhash_find_node(sessionHashTable, &sessionKey);

    if (hnode && hnode->data)
    {
        /* This is a unique hnode, since the sfxhash finds the
         * same key before returning this node.
         */
        returned = (Session *)hnode->data;
    }
    return returned;
}

int RemoveSessionFromHashTable(Session *ssn)
{
    return sfxhash_remove(sessionHashTable, &(ssn->hashKey));
}

int CleanHashTable(u_int32_t thetime, Session *save_me, int memCheck)
{
    Session *idx;
    u_int32_t pruned = 0;

    if (thetime != 0)
    {
        char got_one;
        idx = (Session *) sfxhash_lru(sessionHashTable);

        if(idx == NULL)
        {
            return 0;
        }

        do
        {
            got_one = 0;            
            if(idx == save_me)
            {
                SFXHASH_NODE *lastNode = sfxhash_lru_node(sessionHashTable);
                sfxhash_gmovetofront(sessionHashTable, lastNode);
                lastNode = sfxhash_lru_node(sessionHashTable);
                if ((lastNode) && (lastNode->data != idx))
                {
                    idx = (Session *)lastNode->data;
                    continue;
                }
                else
                {
                    return pruned;
                }
            }

            if((idx->last_session_time+s4data.timeout) < thetime)
            {
                Session *savidx = idx;

                if(sfxhash_count(sessionHashTable) > 1)
                {
                    DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "pruning stale session\n"););
                    DeleteSession(savidx, thetime);
                    idx = (Session *) sfxhash_lru(sessionHashTable);
                    pruned++;
                    got_one = 1;
                }
                else
                {
                    DeleteSession(savidx, thetime);
                    pruned++;
                    return pruned;
                }
            }
            else
            {
                return pruned;
            }
        } while ((idx != NULL) && (got_one == 1));

        return pruned;
    }
    else if (s4data.cache_clean_percent == 0)
    {
        /* Free up xxx sessions at a time until we get under the
         * memcap or free enough sessions to be able to create
         * new ones.
         */
        while ( ((memCheck && (stream4_memory_usage > s4data.memcap)) ||
                 (sessionHashTable->count >
                   (s4data.max_sessions - s4data.cache_clean_sessions))) &&
                (sfxhash_count(sessionHashTable) > 1))
        {
            int i;
            idx = (Session *) sfxhash_lru(sessionHashTable);
            for (i=0;i<s4data.cache_clean_sessions && 
                     (sfxhash_count(sessionHashTable) > 1); i++)
            {
                if(idx != save_me)
                {
                    DeleteSession(idx, thetime);
                    pruned++;
                    idx = (Session *) sfxhash_lru(sessionHashTable);
                }
                else
                {
                    SFXHASH_NODE *lastNode = sfxhash_lru_node(sessionHashTable);
                    sfxhash_gmovetofront(sessionHashTable, lastNode);
                    lastNode = sfxhash_lru_node(sessionHashTable);
                    if ((lastNode) && (lastNode->data == idx))
                    {
                        /* Okay, this session is the only one left */
                        break;
                    }
                    idx = (Session *) sfxhash_lru(sessionHashTable);
                    i--; /* Didn't clean this one */
                }
            }
        }
    }
    else
    {
        /* Free up a percentage of the cache */
        u_int32_t smallPercent = (u_int32_t)(s4data.max_sessions *
                        s4data.cache_clean_percent);
        idx = (Session *) sfxhash_lru(sessionHashTable);
        while ((stream4_memory_usage > (s4data.memcap - smallPercent)) &&
                (sfxhash_count(sessionHashTable) > 1))
        {
            idx = (Session *) sfxhash_lru(sessionHashTable);
            if(idx != save_me)
            {
                DeleteSession(idx, thetime);
                pruned++;
                idx = (Session *) sfxhash_lru(sessionHashTable);
            }
            else
            {
                SFXHASH_NODE *lastNode = sfxhash_lru_node(sessionHashTable);
                sfxhash_gmovetofront(sessionHashTable, lastNode);
                lastNode = sfxhash_lru_node(sessionHashTable);
                if ((lastNode) && (lastNode->data == idx))
                {
                    /* Okay, this session is the only one left */
                    break;
                }
                idx = (Session *) sfxhash_lru(sessionHashTable);
            }
        }
    }
    return pruned;
}

Session *GetNewSession(Packet *p)
{
    Session *retSsn = NULL;
    SessionHashKey sessionKey;
    SFXHASH_NODE *hnode;

    if (!GetSessionKey(p, &sessionKey))
        return retSsn;

    hnode = sfxhash_get_node(sessionHashTable, &sessionKey);
    if (!hnode)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "HashTable full, clean it\n"););
        if (!CleanHashTable(p->pkth->ts.tv_sec, NULL, 0))
        {
            DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "HashTable full, no timeouts, clean it\n"););
            CleanHashTable(0, NULL, 0);
        }

        /* Should have some freed nodes now */
        hnode = sfxhash_get_node(sessionHashTable, &sessionKey);
#ifdef DEBUG
        if (!hnode)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "Problem, no freed nodes\n"););
        }
#endif
    }
    if (hnode && hnode->data)
    {
        retSsn = hnode->data;

        /* Zero everything out */
        memset(retSsn, 0, sizeof(Session));

        /* Save the session key for future use */
        memcpy(&(retSsn->hashKey), &sessionKey,
                        sizeof(SessionHashKey));

#if 0
        retSsn->ttl = 0;
        retSsn->alert_count = 0;
        retSsn->ignore_flag = 0;
        retSsn->preproc_data = NULL;
        retSsn->preproc_free = NULL;
        retSsn->preproc_proto = 0;

        retSsn->client.overlap_pkts = 0;
        retSsn->server.overlap_pkts = 0;
        retSsn->client.bytes_inspected = 0;
        retSsn->server.bytes_inspected = 0;
        retSsn->client.expected_flags = 0;
        retSsn->server.expected_flags = 0;
#endif
    }
    return retSsn;
}
#endif

Session *GetSessionFromSplayTree(Packet *p)
{
    Session idx;
    Session *returned;
#ifdef DEBUG
    char flagbuf[9];
    CreateTCPFlagString(p, flagbuf);
#endif

    DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "Trying to get session...\n"););
    idx.server.ip = p->iph->ip_src.s_addr;
    idx.client.ip = p->iph->ip_dst.s_addr;
    idx.server.port = p->sp;
    idx.client.port = p->dp;

    DEBUG_WRAP(DebugMessage(DEBUG_STREAM,"Looking for sip: 0x%X sp: %d  cip: "
                "0x%X cp: %d flags: %s\n", idx.server.ip, idx.server.port, 
                idx.client.ip, idx.client.port, flagbuf););

    returned = (Session *) ubi_sptFind(RootPtr, (ubi_btItemPtr)&idx);

    if(returned == NULL)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "GetSession forward didn't work, "
                    "trying backwards...\n"););
        idx.server.ip = p->iph->ip_dst.s_addr;
        idx.client.ip = p->iph->ip_src.s_addr;
        idx.server.port = p->dp;
        idx.client.port = p->sp;
        DEBUG_WRAP(DebugMessage(DEBUG_STREAM,"Looking for sip: 0x%X sp: %d  "
                                "cip: 0x%X cp: %d flags: %s\n", idx.server.ip, 
                                idx.server.port, idx.client.ip, idx.client.port,
                                flagbuf););
        returned = (Session *) ubi_sptFind(RootPtr, (ubi_btItemPtr)&idx);
    }

    if(returned == NULL)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "Unable to find session\n"););
    }
    else
    {
        DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "Found session\n"););
    }

    return returned;

}

Session *RemoveSession(Session *ssn)
{
#ifdef USE_HASH_TABLE
    if (!RemoveSessionFromHashTable(ssn) )
        return ssn;
    else
        return NULL;
#else /* USE_SPLAY_TREE */
    Session *killme = NULL;
    if(ubi_trCount(RootPtr))
    {
        killme = (Session *) ubi_sptRemove(RootPtr, (ubi_btNodePtr) ssn);
    }
    return killme;
#endif
}

Session *GetSession(Packet *p)
{
    Session *ssn;
    PROFILE_VARS;
    PREPROC_PROFILE_START(stream4LUSessPerfStats);
#ifdef USE_HASH_TABLE
    ssn = GetSessionFromHashTable(p);
#else /* USE_SPLAY_TREE */
    ssn = GetSessionFromSplayTree(p);
#endif
    PREPROC_PROFILE_END(stream4LUSessPerfStats);
    return ssn;
}

#ifdef USE_HASH_TABLE
#else /* USE_SPLAY_TREE */
static int CompareFunc(ubi_trItemPtr ItemPtr, ubi_trNodePtr NodePtr)
{
    Session *nSession;
    Session *iSession; 

    nSession = ((Session *)NodePtr);
    iSession = (Session *)ItemPtr;

    if(nSession->server.ip < iSession->server.ip) return 1;
    else if(nSession->server.ip > iSession->server.ip) return -1;

    if(nSession->client.ip < iSession->client.ip) return 1;
    else if(nSession->client.ip > iSession->client.ip) return -1;
        
    if(nSession->server.port < iSession->server.port) return 1;
    else if(nSession->server.port > iSession->server.port) return -1;

    if(nSession->client.port < iSession->client.port) return 1;
    else if(nSession->client.port > iSession->client.port) return -1;

    return 0;
}
#endif

void InitSessionCache()
{
#ifdef USE_HASH_TABLE
    if (!sessionHashTable)
    {
        /* Create the hash table --
         * SESSION_HASH_TABLE_SIZE hash buckets
         * keysize = 12 bytes (2x 32bit IP, 2x16bit port)
         * data size = sizeof(Session) object
         * no max mem
         * no automatic node recovery
         * NULL node recovery free function
         * NULL user data free function
         * recycle nodes
         */
        /* Rule of thumb, size should be 1.4 times max to avoid
         * collisions.
         */
        int hashTableSize = (int) (s4data.max_sessions * 1.4);
        int maxSessionMem = s4data.max_sessions * (
                             sizeof(Session) +
                             sizeof(SFXHASH_NODE) +
                             sizeof(SessionHashKey) +
                             sizeof (SFXHASH_NODE *));
        int tableMem = (hashTableSize +1) * sizeof(SFXHASH_NODE*);
        int maxMem = maxSessionMem + tableMem;
        sessionHashTable = sfxhash_new(hashTableSize,
                        sizeof(SessionHashKey),
                        sizeof(Session), maxMem, 0, NULL, NULL, 1);


    }
#else /* USE_SPLAY_TREE */
    (void)ubi_trInitTree(RootPtr,       /* ptr to the tree head */
                         CompareFunc,   /* comparison function */
                         0);            /* don't allow overwrites/duplicates */

#endif
}

void PurgeSessionCache()
{
    Session *ssn = NULL;
#ifdef USE_HASH_TABLE
    ssn = (Session *)sfxhash_mru(sessionHashTable);
#else /* USE_SPLAY_TREE */
    ssn = (Session *)ubi_trFirst(RootPtr);
#endif
    while (ssn)
    {
        DeleteSession(ssn, 0);
#ifdef USE_HASH_TABLE
        ssn = (Session *)sfxhash_mru(sessionHashTable);
#else /* USE_SPLAY_TREE */
        ssn = (Session *)ubi_trFirst(RootPtr);
#endif
    }
}

void PrintSessionCache()
{
#ifdef USE_HASH_TABLE
    DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "%lu streams active, %u bytes in use\n", 
                            sfxhash_count(sessionHashTable), stream4_memory_usage););
#else /* USE_SPLAY_TREE */
    DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "%lu streams active, %u bytes in use\n", 
                            ubi_trCount(RootPtr), stream4_memory_usage););
#endif
    return;
}

int PruneSessionCache(u_int32_t thetime, int mustdie, Session *save_me)
{
#ifdef USE_HASH_TABLE
    return CleanHashTable(thetime, save_me, 1);
#else /* USE_SPLAY_TREE */
    Session *idx;
    u_int32_t pruned = 0;

    if(ubi_trCount(RootPtr) == 0)
    {
        return 0;
    }

    {
        if (thetime != 0)
        {
            char got_one;
            idx = (Session *) ubi_btLast((ubi_btNodePtr)RootPtr->root);

            if(idx == NULL)
            {
                return 0;
            }

            do
            {
                got_one = 0;            
                if(idx == save_me)
                {
                    idx = (Session *) ubi_btPrev((ubi_btNodePtr)idx);
                    continue;
                }

                if((idx->last_session_time+s4data.timeout) < thetime)
                {
                    Session *savidx = idx;

                    if(ubi_trCount(RootPtr) > 1)
                    {
                        idx = (Session *) ubi_btPrev((ubi_btNodePtr)idx);
                        DEBUG_WRAP(DebugMessage(DEBUG_STREAM, "pruning stale session\n"););
                        DeleteSession(savidx, thetime);
                        pruned++;
                        got_one = 1;
                    }
                    else
                    {
                        DeleteSession(savidx, thetime);
                        pruned++;
                        return pruned;
                    }
                }
                else
                {
                    if(idx != NULL && ubi_trCount(RootPtr))
                    {
                        idx = (Session *) ubi_btPrev((ubi_btNodePtr)idx);
                    }
                    else
                    {
                        return pruned;
                    }
                }
            } while ((idx != NULL) && (got_one == 1));

            return pruned;
        }
        else if (s4data.cache_clean_percent == 0)
        {
            /* Free up xxx sessions at a time until we get under the
             * memcap */
            while ((stream4_memory_usage > s4data.memcap) &&
                   ubi_trCount(RootPtr) > 1)
            {
                int i;
                idx = (Session *) ubi_btLeafNode((ubi_btNodePtr)RootPtr);
                for (i=0;i<s4data.cache_clean_sessions && 
                         ubi_trCount(RootPtr) > 1; i++)
                {
                    if(idx != save_me)
                    {
                        DeleteSession(idx, thetime);
                        pruned++;
                        idx = (Session *) ubi_btLeafNode((ubi_btNodePtr)RootPtr);
                    }
                    else
                    {
                        Rotate((ubi_btNodePtr)idx);
                        idx = (Session *) ubi_btLeafNode((ubi_btNodePtr)RootPtr);
                        i--; /* Didn't clean this one */
                    }
                }
            }
        }
        else
        {
            /* Free up a percentage of the cache */
            u_int32_t smallPercent = (u_int32_t)(s4data.memcap * s4data.cache_clean_percent);
            idx = (Session *) ubi_btLeafNode((ubi_btNodePtr)RootPtr);
            while ((stream4_memory_usage > (s4data.memcap - smallPercent)) &&
                   ubi_trCount(RootPtr) > 1)
            {
                if(idx != save_me)
                {
                    DeleteSession(idx, thetime);
                    pruned++;
                    idx = (Session *) ubi_btLeafNode((ubi_btNodePtr)RootPtr);
                }
                else
                {
                    Rotate((ubi_btNodePtr)idx);
                    idx = (Session *) ubi_btLeafNode((ubi_btNodePtr)RootPtr);
                }
            }
        }
#ifdef DEBUG
        if(ubi_trCount(RootPtr) == 1) {
            DebugMessage(DEBUG_STREAM, "Emptied out the stream cache "
                         "completely mustdie: %d, memusage: %u\n",
                         mustdie,
                         stream4_memory_usage);
        }
#endif /* DEBUG */
        return pruned;
    }
#endif /* USE_HASH_TABLE */

    return 0;
}

