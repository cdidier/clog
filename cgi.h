/* $Id$ */

#ifndef CGI_H
#define CGI_H

#include <sys/queue.h>

struct query_param {
        char    *key;
	char    *value;
	SLIST_ENTRY(query_param) next;
};

struct query {
	char	*query_str;
	SLIST_HEAD(, query_param) params;
};

struct query	*tokenize_query(const char *);
void		 free_query(struct query *);
char		*get_query_param(struct query *, const char *);

#endif
