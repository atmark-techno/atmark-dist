#ifndef SIMPLECGIAPPLLIST_H_
#define SIMPLECGIAPPLLIST_H_

typedef struct cgi_str_list {
	char *str;
	struct cgi_str_list *next_list;
} cgiStrList;

typedef struct cgi_str_pair_list {
	char *name;
	char *val;
	struct cgi_str_pair_list *next_list;
} cgiStrPairList;

typedef struct cgi_str_pair_ll {
	cgiStrPairList *str_pair_list;
	struct cgi_str_pair_ll *next_list;
} cgiStrPairLL;

/* allows duplicates */
extern void cgi_str_list_add(cgiStrList **list_current, char *str_to_add);
extern void cgi_str_list_add_to_start(cgiStrList **list_current, char *str_to_add);
/* doesn't allow duplicates */
extern void cgi_str_list_add_unique(cgiStrList **list_current, char *str_to_add);
extern void cgi_str_list_add_unique_ordered(cgiStrList **list_current, char *str_to_add);
extern cgiStrList *cgi_str_list_next(cgiStrList *list_current);
extern void cgi_str_list_free(cgiStrList *list_start);
extern int cgi_str_list_exists(cgiStrList *list_start, char *str);
extern int cgi_str_list_length(cgiStrList *list_start);

/* doesn't allow duplicates */
extern void cgi_str_pair_list_add(cgiStrPairList **list_current,
		char *name_to_add, char *val_to_add);
extern void cgi_str_pair_list_free(cgiStrPairList *list_start);
extern cgiStrPairList *cgi_str_pair_list_next(cgiStrPairList *list_current);
extern char *cgi_str_pair_list_get_val(cgiStrPairList *list_start, char *name);

/* allows duplicates */
extern cgiStrPairLL *cgi_str_pair_ll_add(cgiStrPairLL **list_current, cgiStrPairList *list_to_add);
extern void cgi_str_pair_ll_free(cgiStrPairLL *list_start);
extern cgiStrPairLL *cgi_str_pair_ll_next(cgiStrPairLL *list_current);
extern cgiStrPairList *cgi_str_pair_ll_get(cgiStrPairLL *list_start, int index);
extern int cgi_str_pair_ll_delete(cgiStrPairLL **list_start, int index);
extern int cgi_str_pair_ll_replace(cgiStrPairLL **list_start, int index, cgiStrPairList *new_list);
#endif /*SIMPLECGIAPPLLIST_H_*/
