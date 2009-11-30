/*
 * $Id$
 *
 * Copyright (c) 2009 Colin Didier <cdidier@cybione.org>
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

#ifndef LINT
static char rcsid[] = "$Id$";
#endif

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

#include "common.h"
#include "articles.h"

/* article filename format: YYYYMMDDHHmm... */
#define ARTICLE_NAME_FORMAT "%Y%m%d%H%M"
#define ARTICLE_NAME_MINLEN 12

int
is_article_name(const char *article, size_t len)
{
	if ((strncmp(article, "20", 2) == 0 || strncmp(article, "19", 2) == 0)
	    && len >= ARTICLE_NAME_MINLEN)
		return 1;
	return 0;
}

int
read_article(const char *article, article_cb *callback)
{
	struct article a;
	char path[MAXPATHLEN];
	char *buf;
	size_t len;
	struct article_tag *at;
	struct stat sb;
	extern enum STATUS status;

	/* extract the date */
	memset(&a.date, 0, sizeof(struct tm));
	if (strptime(article, ARTICLE_NAME_FORMAT, &a.date) == NULL)
		return -1;
	mktime(&a.date);
	/* open the content */
	snprintf(path, MAXPATHLEN, "%s" ARTICLES_DIR "/%s/article",
	    status & STATUS_FROMCMD ? CHROOT_DIR : "", article);
	if ((a.body = fopen(path, "r")) == NULL) {
		if (errno != ENOENT)
			warn("fopen: %s", path);
		return -1;
	}
	/* extract the title */
	if ((buf = fgetln(a.body, &len)) == NULL && !feof(a.body)) {
		warn("fgets: %s", path);
		fclose(a.body);
		return -1;
	}
	if (callback != NULL) {
		buf[strcspn(buf, "\n")] = '\0';
		if ((a.title = strdup(buf)) == NULL) {
			warn("strdup");
			return -1;
		}
		/* open more if available */
		snprintf(path, MAXPATHLEN, "%s" ARTICLES_DIR "/%s/more",
		    status & STATUS_FROMCMD ? CHROOT_DIR : "", article);
		a.more = fopen(path, "r");
		if (a.more != NULL)
			a.more_size = stat(path, &sb) != -1 ? sb.st_size : 0;
		a.display_more = 0;
		a.antispam = NULL;
		SLIST_INIT(&a.tags);
		SLIST_FIRST(&a.tags) = get_article_tags(article);
		a.name = article;
		callback(&a);
		free(a.title);
		if (a.more != NULL)
			fclose(a.more);
		while (!SLIST_EMPTY(&a.tags)) {
			at = SLIST_FIRST(&a.tags);
			SLIST_REMOVE_HEAD(&a.tags, next);
			free(at->name);
			free(at);
		}
	}
	fclose(a.body);
	return 0;
}

static int
compar_fts_name_desc(const FTSENT **f1, const FTSENT **f2)
{
        return strcmp((*f2)->fts_name, (*f1)->fts_name);
}

unsigned long
read_articles(const char *tag, unsigned long offset, unsigned long number,
    article_cb *callback)
{
	FTS *fts;
	FTSENT *e;
	char path[MAXPATHLEN];
	char * const path_argv[] = { path, NULL };
	unsigned long nb_articles, nb_skipped;
	extern enum STATUS status;

	nb_articles = 0;
	if (tag != NULL)
		snprintf(path, MAXPATHLEN, "%s" TAGS_DIR "/%s",
		    status & STATUS_FROMCMD ? CHROOT_DIR : "", tag);
	else
		snprintf(path, MAXPATHLEN, "%s" ARTICLES_DIR,
		    status & STATUS_FROMCMD ? CHROOT_DIR : "");
	if ((fts = fts_open(path_argv, FTS_LOGICAL|FTS_NOSTAT,
	    compar_fts_name_desc)) == NULL) {
		warn("fts_open: %s", path);
		return 0;
	} else if ((e = fts_read(fts)) == NULL || !(e->fts_info & FTS_D)) {
		if (errno != ENOENT)
			warn("fts_read: %s", path);
		goto out;
	} else if ((e = fts_children(fts, 0)) == NULL) {
		if (errno != 0)
			warn("fts_children: %s", path);
		goto out;
	}
	if (number == 0)
		number = ULONG_MAX;
	for (nb_skipped = 0; e != NULL && nb_articles < number;
	    e = e->fts_link) {
		if (is_article_name(e->fts_name, e->fts_namelen)) {
			if (nb_skipped < offset) {
				if (read_article(e->fts_name, NULL) != -1)
					++nb_skipped;
				continue;
			}
			read_article(e->fts_name, callback);
			++nb_articles;
		}
	}
out:	fts_close(fts);
	return nb_articles;
}

struct article_tag *
get_article_tags(const char *article)
{
	FTS *fts;
	FTSENT *e;
	char path[MAXPATHLEN];
	char * const path_argv[] = { path, NULL };
	char *tag;
	struct article_tag *at;
	SLIST_HEAD(, article_tag) list;
	unsigned long number;
	int index;
	extern enum STATUS status;

	SLIST_INIT(&list);
	index = 0;
	snprintf(path, MAXPATHLEN, "%s" TAGS_DIR,
	    status & STATUS_FROMCMD ? CHROOT_DIR : "");
again:	if ((fts = fts_open(path_argv, FTS_LOGICAL|FTS_NOSTAT,
	    compar_fts_name_desc)) == NULL) {
		warn("fts_open: %s", path);
		return NULL;
	} else if ((e = fts_read(fts)) == NULL || !(e->fts_info & FTS_D)) {
		if (errno != ENOENT)
			warn("fts_read: %s", path);
		goto out;
	}
	number = 0;
	for (; (e = fts_read(fts)) != NULL;) {
		tag = NULL;
		if (article != NULL) {
			if (e->fts_level == (2 - index)) {
				if (e->fts_info & FTS_D) {
					fts_set(fts, e, FTS_SKIP);
					fts_read(fts);
				}
				++number;
				if (strcmp(e->fts_name, article) == 0)
					tag = e->fts_parent->fts_name;
			} else
				number = 0;
		} else if (e->fts_info & FTS_D) {
			if (e->fts_level == 1)
				fts_set(fts, e, FTS_SKIP);
			tag = e->fts_name;
		}
		if (tag == NULL)
			continue;
		if ((at = malloc(sizeof(struct article_tag))) == NULL) {
			warn("malloc");
			continue;
		}
		if (index == 0) {
			if ((at->name = strdup(tag)) == NULL) {
				warn("strdup");
				free(at);
				continue;
			}
		} else
			at->name = NULL;
		at->number = number;
		SLIST_INSERT_HEAD(&list, at, next);
	}
	/* try again to get the articles number in the index */
	if (article != NULL && index == 0) {
		index = 1;
		snprintf(path, MAXPATHLEN, "%s" ARTICLES_DIR,
		     status & STATUS_FROMCMD ? CHROOT_DIR : "");
		goto again;
	}
out:	fts_close(fts);
	return SLIST_FIRST(&list);
}

int
read_tag(const char *tag, unsigned long page, unsigned long number,
    tag_cb *callback)
{
	struct tag t;
	unsigned long nb_articles;

	if ((nb_articles = read_articles(tag, 0, 0, NULL)) == 0)
		if (tag != NULL || page != 0)
			return -1;
	t.pages = nb_articles/number + (nb_articles%number != 0 ? 1 : 0);
	if (page > t.pages-1) {
		if (tag != NULL || page != 0)
			return -1;
	}
	t.next = (page != t.pages-1);
	t.previous = (page > 0);
	if (page && number > ULONG_MAX / page)
		if (tag != NULL || page != 0)
			return -1;
	t.offset = page != 0 ? page * number : 0;
	if (callback != NULL) {
		t.name = tag;
		t.page = page;
		t.number = number;
		callback(&t);
	}
	return 0;
}
