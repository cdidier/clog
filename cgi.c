/*
 * $Id$
 *
 * Copyright (c) 2008,2009 Colin Didier <cdidier@cybione.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <assert.h>
#include <err.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include "cgi.h"

#define EMPTYSTRING(x)	((x) == NULL || (*(x) == '\0'))

static const char *empty_str = "";

static void
url_decode(char *src)
{
        static const char *hex = "0123456789abcdef";
	char *dest, *i;

	assert(src != NULL);
	for(dest = src; *src != '\0'; src++, dest++) {
		if (*src == '%' && *++src != '\0'
		    && (i = strchr(hex, tolower(*src))) != NULL) {
			*dest = (i - hex) * 16;
			if (*++src != '\0'
			    && (i = strchr(hex, tolower(*src))) != NULL)
				*dest += i - hex;
		} else
			*dest = *src  == '+' ? ' ' : *src;
	}
	*dest = '\0';
}

struct query *
tokenize_query(const char *query_str)
{
	char *next, *key, *value;
	struct query *q;
	struct query_param *qp;

	if (query_str == NULL)
		return NULL;
	if ((q = malloc(sizeof(struct query))) == NULL) {
		warn("malloc");
		return NULL;
	}
	SLIST_INIT(&q->params);
	if ((q->query_str = strdup(query_str)) == NULL) {
		warn("strdup");
		return q;
	}
	for (key = q->query_str; !EMPTYSTRING(key); key = next) {
		if ((next = strpbrk(key, "&;")) != NULL)
			*next++ = '\0';
		if ((value = strchr(key, '=')) != NULL)
			*value++ = '\0';
		if (*key == '\0')
			continue;
		url_decode(key);
		if (!EMPTYSTRING(value))
			url_decode(value);
		if ((qp = malloc(sizeof(struct query_param))) == NULL) {
			warn("malloc");
			continue;
		}
		qp->key = key;
		qp->value = value == NULL ? (char *)empty_str : value;
		SLIST_INSERT_HEAD(&q->params, qp, next);
	}
	return q;
}

char *
get_query_param(struct query *q, const char *key)
{
	struct query_param *qp;

	assert(key != NULL);
	if (q == NULL)
		return NULL;
	SLIST_FOREACH(qp, &q->params, next) {
		if (strcasecmp(qp->key, key) == 0)
			return qp->value;
	}
	return NULL;
}

void
free_query(struct query *q)
{
	struct query_param *qp;

	if (q == NULL)
		return;
	while (!SLIST_EMPTY(&q->params)) {
		qp = SLIST_FIRST(&q->params);
		SLIST_REMOVE_HEAD(&q->params, next);
		free(qp);
	}
	free(q->query_str);
	free(q);
}
