/*
 * $Id$
 *
 * Copyright (c) 2008 Colin Didier <cdidier@cybione.org>
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
#include <sys/param.h>
#include <sys/stat.h>
#include <fts.h>

#include "common.h"

static void
read_comment(char *aname, char *cname, comment_cb cb)
{
	FILE *fin;
	char path[MAXPATHLEN], buf[BUFSIZ];
	char author[COMMENT_MAXLEN], ip[16], mail[COMMENT_MAXLEN],
	    web[COMMENT_MAXLEN];
	struct tm tm;

	snprintf(path, MAXPATHLEN, ARTICLES_DIR"/%s/comments/%s", aname,
	    cname);
	if ((fin = fopen(path, "r")) == NULL) {
		warn("fopen: %s", path);
		return;
	}
	memset(&tm, 0, sizeof(struct tm));
	if (strptime(cname, FILE_FORMAT, &tm) == NULL)
		return;
	*author = *ip = *mail = *web = '\0';
	while (fgets(buf, BUFSIZ, fin) != NULL && *buf != '\n') {
		buf[strcspn(buf, "\n")] = '\0';
		if (strncmp(buf, COMMENT_AUTHOR,
		    sizeof(COMMENT_AUTHOR)-1) == 0)
			strlcpy(author, buf+sizeof(COMMENT_AUTHOR)-1,
			    COMMENT_MAXLEN);
		else if (strncmp(buf, COMMENT_IP,
		    sizeof(COMMENT_IP)-1) == 0)
			strlcpy(ip, buf+sizeof(COMMENT_IP)-1, 16);
		else if (strncmp(buf, COMMENT_MAIL,
		    sizeof(COMMENT_MAIL)-1) == 0)
			strlcpy(mail, buf+sizeof(COMMENT_MAIL)-1,
			    COMMENT_MAXLEN);
		else if (strncmp(buf, COMMENT_WEB,
		    sizeof(COMMENT_WEB)-1) == 0)
			strlcpy(web, buf+sizeof(COMMENT_WEB)-1,
			    COMMENT_MAXLEN);
	}
	if (*author != '\0' && cb != NULL)
		cb(author, &tm, ip, mail, web, fin);
	fclose(fin);
}

static int
compar_name_asc(const FTSENT **f1, const FTSENT **f2)
{
	return strcmp((*f1)->fts_name, (*f2)->fts_name);
}

uint
read_comments(char *aname, comment_cb cb)
{
	FTS *fts;
	FTSENT *e;
	char path[MAXPATHLEN];
	char * const path_argv[] = { path, NULL };
	uint nb;

	nb = 0;
	snprintf(path, MAXPATHLEN, ARTICLES_DIR"/%s/comments", aname);
	if ((fts = fts_open(path_argv, FTS_LOGICAL|FTS_NOSTAT,
	    compar_name_asc)) == NULL) {
		warn("fts_open: %s", path);
		return 0;
	}
	else if ((e = fts_read(fts)) == NULL || !(e->fts_info & FTS_D)) {
		if (errno != ENOENT)
			warn("fts_read: %s", path);
		goto out;
	} else if ((e = fts_children(fts, FTS_NAMEONLY)) == NULL) {
		warn("fts_children: %s", path);
		goto out;
	}
	for (; e != NULL; e = e->fts_link) {
		if (cb != NULL)
			read_comment(aname, e->fts_name, cb);
		++nb;
	}
out:	fts_close(fts);	
	return nb;
}

uint
read_article_tags(char *aname, article_tag_cb cb)
{
	FTS *fts, *fts2;
	FTSENT *e, *e2;
	char * const path_argv[] = { TAGS_DIR, NULL };
	char path[MAXPATHLEN];
	char * const path2_argv[] = { path, NULL };
	uint nb;

	nb = 0;
	if ((fts = fts_open(path_argv, FTS_LOGICAL|FTS_NOSTAT,
	    compar_name_asc)) == NULL) {
		warn("fts_open: "TAGS_DIR);
		return 0;
	} else if ((e = fts_read(fts)) == NULL || !(e->fts_info & FTS_D)) {
		if (errno != ENOENT)
			warn("fts_read: "TAGS_DIR);
		goto out;
	} else if ((e = fts_children(fts, FTS_NAMEONLY)) == NULL) {
		warn("fts_children: "TAGS_DIR);
		goto out;
	}
	for (; e != NULL; e = e->fts_link) {
		snprintf(path, MAXPATHLEN, "%s/%s", e->fts_accpath,
		    e->fts_name);
		if ((fts2 = fts_open(path2_argv, FTS_LOGICAL|FTS_NOSTAT,
		    compar_name_asc)) == NULL) {
			warn("fts_open: %s", path);
			continue;
		} else if ((e2 = fts_read(fts2)) == NULL
		    || !(e2->fts_info & FTS_D)) {
			if (errno != ENOENT)
				warn("fts_read: %s", path);
			goto out2;
		} else if ((e2 = fts_children(fts2, FTS_NAMEONLY)) == NULL) {
			warn("fts_children: %s", path);
			goto out2;
		}
		for (; e2 != NULL; e2 = e2->fts_link)
			if (strcmp(e2->fts_name, aname) == 0) {
				if (cb != NULL)
					cb(e->fts_name);
				++nb;
				goto out2;
			}
out2:		fts_close(fts2);
	}
out:	fts_close(fts);	
	if (nb == 0 && cb != NULL)
		cb(NULL);
	return nb;
}

int
read_article(char *aname, article_cb cb, char *atitle, size_t atitle_len)
{
	FILE *fin;
	char path[MAXPATHLEN], title[BUFSIZ];
	struct tm tm;

	memset(&tm, 0, sizeof(struct tm));
	if (strptime(aname, FILE_FORMAT, &tm) == NULL)
		return -1;
	snprintf(path, MAXPATHLEN, ARTICLES_DIR"/%s/article", aname);
	if ((fin = fopen(path, "r")) == NULL) {
		if (errno != ENOENT)
			warn("fopen: %s", path);
		return -1;
	}
	if (fgets(title, BUFSIZ, fin) == NULL && !feof(fin)) {
		warn("fgets: %s", path);
		fclose(fin);
		return -1;
	}
	title[strcspn(title, "\n")] = '\0';
	if (atitle != NULL)
		strlcpy(atitle, title, atitle_len);
	if (cb != NULL)
		cb(aname, title, &tm, fin, read_comments(aname, NULL));
	fclose(fin);
	return 0;
}

static int
look_like_an_article(FTSENT *e)
{
	return (e->fts_info & FTS_D) && e->fts_namelen >= FILE_MINLEN
	    && (strncmp(e->fts_name, "20", 2) == 0
	    || strncmp(e->fts_name, "19", 2) == 0);
}

static int
compar_name_desc(const FTSENT **f1, const FTSENT **f2)
{
	return strcmp((*f2)->fts_name, (*f1)->fts_name);
}

void
read_articles(article_cb cb)
{
	FTS *fts;
	FTSENT *e;
	char path[MAXPATHLEN];
	char * const path_argv[] = { path, NULL };
	long i;
	extern char *tag;
	extern long offset;

	if (tag != NULL)
		snprintf(path, MAXPATHLEN, TAGS_DIR"/%s", tag);
	else
		strlcpy(path, ARTICLES_DIR, MAXPATHLEN);
	if ((fts = fts_open(path_argv, FTS_LOGICAL|FTS_NOSTAT,
	    compar_name_desc)) == NULL) {
		warn("fts_open: %s", path);
		return;
	} else if ((e = fts_read(fts)) == NULL || !(e->fts_info & FTS_D)) {
		if (errno != ENOENT)
			warn("fts_read: %s", path);
		goto out;
	} else if ((e = fts_children(fts, FTS_NAMEONLY)) == NULL) {
		warn("fts_children: %s", path);
		goto out;
	}
	for (i = 0; e != NULL && i++ < offset*NB_ARTICLES;)
		if (look_like_an_article(e))
			e = e->fts_link;
	for (i = 0; e != NULL && i++ < NB_ARTICLES; e = e->fts_link) {
		if (!look_like_an_article(e))
	    		continue;
		read_article(e->fts_name, cb, NULL, 0);
	}
out:	fts_close(fts);	
}

void
read_tags(tag_cb cb)
{
	FTS *fts, *fts2;
	FTSENT *e, *e2;
	char * const path_argv[] = { TAGS_DIR, NULL };
	char path[MAXPATHLEN];
	char * const path2_argv[] = { path, NULL };
	uint nb;

	if ((fts = fts_open(path_argv, FTS_LOGICAL|FTS_NOSTAT,
	    compar_name_asc)) == NULL) {
		warn("fts_open: "TAGS_DIR);
		return;
	} else if ((e = fts_read(fts)) == NULL || !(e->fts_info & FTS_D)) {
		if (errno != ENOENT)
			warn("fts_read: "TAGS_DIR);
		goto out;
	} else if ((e = fts_children(fts, FTS_NAMEONLY)) == NULL) {
		warn("fts_children: "TAGS_DIR);
		goto out;
	}
	for (; e != NULL; e = e->fts_link) {
		snprintf(path, MAXPATHLEN, "%s/%s", e->fts_accpath,
		    e->fts_name);
		if ((fts2 = fts_open(path2_argv, FTS_LOGICAL|FTS_NOSTAT,
		    compar_name_asc)) == NULL) {
			warn("fts_open: %s", path);
			continue;
		} else if ((e2 = fts_read(fts2)) == NULL
		    || !(e2->fts_info & FTS_D)) {
			if (errno != ENOENT)
				warn("fts_read: %s", path);
			goto out2;
		} else if ((e2 = fts_children(fts2, FTS_NAMEONLY)) == NULL) {
			warn("fts_children: %s", path);
			goto out2;
		}
		for (nb = 0; e2 != NULL; e2 = e2->fts_link)
				++nb;
		if (cb != NULL)
			cb(e->fts_name, nb);
out2:		fts_close(fts2);
	}
out:	fts_close(fts);	
}
