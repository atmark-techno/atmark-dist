#ifndef SNORT_STREAM4_SESSION_H_
#define SNORT_STREAM4_SESSION_H_

#ifdef USE_HASH_TABLE
void InitSessionCache();
void PurgeSessionCache();
Session *GetSession(Packet *);
//Session *InsertSession(Packet *, Session *);
Session *GetNewSession(Packet *);
Session *RemoveSession(Session *);
void PrintSessionCache();
int PruneSessionCache(u_int32_t thetime, int mustdie, Session *save_me);
int GetSessionCount();
#endif

#if defined(USE_HASH_TABLE) || defined(USE_SPLAY_TREE)
Session *GetSession(Packet *);
#endif

#endif /* SNORT_STREAM4_SESSION_H_ */

