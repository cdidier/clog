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

#ifdef ENABLE_STATIC
extern int from_cmd;
#endif /* ENABLE_STATIC */

struct data_foreach {
	const char	*name;
	union {
		ulong	unb;
		long	nb;
		time_t	time;
	};
	union {
		comment_cb	*c_cb;
		article_tag_cb	*at_cb;
		article_cb	*a_cb;
	};
};

static int
compar_name_asc(const FTSENT **f1, const FTSENT **f2)
{
	return strcmp((*f1)->fts_name, (*f2)->fts_name);
}

static int
compar_name_desc(const FTSENT **f1, const FTSENT **f2)
{
	return strcmp((*f2)->fts_name, (*f1)->fts_name);
}

#ifdef ENABLE_COMMENTS

int
foreach_comment(const char *aname, foreach_comment_cb cb, void *data)
{
	FTS *fts;
	FTSENT *e;
	char path[MAXPATHLEN];
	char * const path_argv[] = { path, NULL };

#ifdef ENABLE_STATIC
	if (from_cmd)
		snprintf(path, MAXPATHLEN, CHROOT_DIR ARTICLES_DIR
		    "/%s/comments", aname);
	else
#endif /* ENABLE_STATIC */
		snprintf(path, MAXPATHLEN, ARTICLES_DIR"/%s/comments", aname);
	if ((fts = fts_open(path_argv, FTS_LOGICAL|FTS_NOSTAT,
	    compar_name_asc)) == NULL) {
		warn("fts_open: %s", path);
		return 0;
	} else if ((e = fts_read(fts)) == NULL || !(e->fts_info & FTS_D)) {
		if (errno != ENOENT)
			warn("fts_read: %s", path);
		goto err;
	} else if ((e = fts_children(fts, FTS_NAMEONLY)) == NULL) {
		if (errno != 0)
			warn("fts_children: %s", path);
		goto err;
	}
	for (; e != NULL; e = e->fts_link)
		if (!cb(aname, e->fts_name, data))
			goto err;
	fts_close(fts);
	return 1;
err:	fts_close(fts);
	return 0;
}

#endif /* ENABLE_COMMENTS */

int
foreach_article(foreach_article_cb cb, void *data)
{
	FTS *fts;
	FTSENT *e;
	char path[MAXPATHLEN];
	char * const path_argv[] = { path, NULL };
	extern const char *tag;

	if (tag != NULL) {
#ifdef ENABLE_STATIC
		if (from_cmd)
			snprintf(path, MAXPATHLEN, CHROOT_DIR TAGS_DIR
			    "/%s", tag);
		else
#endif /* ENABLE_STATIC */
			snprintf(path, MAXPATHLEN, TAGS_DIR"/%s", tag);
	} else {
#ifdef ENABLE_STATIC
		if (from_cmd)
			strlcpy(path, CHROOT_DIR ARTICLES_DIR, MAXPATHLEN);
		else
#endif /* ENABLE_STATIC */
			strlcpy(path, ARTICLES_DIR, MAXPATHLEN);
	}
	if ((fts = fts_open(path_argv, FTS_LOGICAL|FTS_NOSTAT,
	    compar_name_desc)) == NULL) {
		warn("fts_open: %s", path);
		return 0;
	} else if ((e = fts_read(fts)) == NULL || !(e->fts_info & FTS_D)) {
		if (errno != ENOENT)
			warn("fts_read: %s", path);
		goto err;
	} else if ((e = fts_children(fts, FTS_NAMEONLY)) == NULL) {
		if (errno != 0)
			warn("fts_children: %s", path);
		goto err;
	}
	for (; e != NULL; e = e->fts_link)
		/* if it looks like an article */
		if ((e->fts_info & FTS_D) && e->fts_namelen >= FILE_MINLEN
		    && (strncmp(e->fts_name, "20", 2) == 0)
		    && (strncmp(e->fts_name, "20", 2) == 0))
			if (!cb(e->fts_name, data))
				goto err;
	fts_close(fts);
	return 1;
err:	fts_close(fts);
	return 0;
}

int
foreach_tag(foreach_tag_cb cb, void *data)
{
	FTS *fts;
	FTSENT *e;
	char path[MAXPATHLEN];
	char * const path_argv[] = { path, NULL };

#ifdef ENABLE_STATIC
	if (from_cmd)
		strlcpy(path, CHROOT_DIR TAGS_DIR, MAXPATHLEN);
	else
#endif /* ENABLE_STATIC */
		strlcpy(path, TAGS_DIR, MAXPATHLEN);
	if ((fts = fts_open(path_argv, FTS_LOGICAL|FTS_NOSTAT,
	    compar_name_asc)) == NULL) {
		warn("fts_open: "TAGS_DIR);
		return 0;
	} else if ((e = fts_read(fts)) == NULL || !(e->fts_info & FTS_D)) {
		if (errno != ENOENT)
			warn("fts_read: "TAGS_DIR);
		goto err;
	} else if ((e = fts_children(fts, FTS_NAMEONLY)) == NULL) {
		if (errno != 0)
			warn("fts_children: "TAGS_DIR);
		goto err;
	}
	for (; e != NULL; e = e->fts_link) {
		if (*e->fts_name == '.')
			continue;
		if (!cb(e->fts_name, data))
			goto err;
	}
	fts_close(fts);
	return 1;
err:	fts_close(fts);
	return 0;
}

#ifdef ENABLE_COMMENTS

/*
 *
 */

static void
read_comment(const char *aname, const char *cname, comment_cb cb)
{
	FILE *fin;
	char path[MAXPATHLEN], buf[BUFSIZ];
	char author[COMMENT_MAXLEN], ip[16], mail[COMMENT_MAXLEN],
	    web[COMMENT_MAXLEN];
	struct tm tm;

#ifdef ENABLE_STATIC
	if (from_cmd)
		snprintf(path, MAXPATHLEN, CHROOT_DIR ARTICLES_DIR
		    "/%s/comments/%s", aname, cname);
	else
#endif /* ENABLE_STATIC */
		snprintf(path, MAXPATHLEN, ARTICLES_DIR"/%s/comments/%s", aname,
		    cname);
	if ((fin = fopen(path, "r")) == NULL) {
		warn("fopen: %s", path);
		return;
	}
	memset(&tm, 0, sizeof(struct tm));
	if (strptime(cname, FILE_FORMAT, &tm) == NULL)
		return;
	mktime(&tm);
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

/*
 *
 */

int
do_read_comments(const char *aname, const char *cname, void *data)
{
	struct data_foreach *d = data;

	if (d->c_cb != NULL)
		read_comment(aname, cname, d->c_cb);
	++d->unb;
	return 1;
}

ulong
read_comments(const char *aname, comment_cb cb)
{
	struct data_foreach d;

	d.c_cb = cb;
	d.unb = 0;
	foreach_comment(aname, do_read_comments, &d);
	return d.unb;
}

#endif /* ENABLE_COMMENTS */

/*
 *
 */

static int
do2_read_article_tags(const char *aname, void *data)
{
	struct data_foreach *d = data;
	extern const char *tag;

	if (strcmp(d->name, aname) == 0) {
		d->at_cb(tag);
		++d->unb;
	}
	return 1;
}

static int
do_read_article_tags(const char *tname, void *data)
{
	const char *old_tag;
	extern const char *tag;

	old_tag = tag;
	tag = tname;
	foreach_article(do2_read_article_tags, data);
	tag = old_tag;
	return 1;
}

ulong
read_article_tags(const char *aname, article_tag_cb cb)
{
	struct data_foreach d;

	d.name = aname;
	d.unb = 0;
	d.at_cb = cb;
	foreach_tag(do_read_article_tags, &d);
	return d.unb;
}

#ifdef ENABLE_STATIC

/*
 *
 */

int
do2_get_mtime_tags(const char *tname, void *data)
{
	char path[MAXPATHLEN];
	struct data_foreach *d = data;
	struct stat sb;
	extern const char *tag;

	snprintf(path, MAXPATHLEN, CHROOT_DIR TAGS_DIR
	    "/%s/%s", tag, d->name);
	if (stat(path, &sb) != -1 && sb.st_mtime > d->time)
		d->time = sb.st_mtime;
	return 1;
}

int
do_get_mtime_tags(const char *tname, void *data)
{
	const char *old_tag;
	extern const char *tag;

	old_tag = tag;
	tag = tname;
	foreach_article(do2_get_mtime_tags, data);
	tag = old_tag;
	return 1;
}

int
do_get_mtime_comments(const char *aname, const char *cname, void *data)
{
	char path[MAXPATHLEN];
	struct stat sb;
	time_t *mtime = data;

	snprintf(path, MAXPATHLEN, CHROOT_DIR ARTICLES_DIR
	    "/%s/comments/%s", aname, cname);
	if (stat(path, &sb) != -1 && sb.st_mtime > *mtime)
		*mtime = sb.st_mtime;
	return 1;
}

time_t
get_mtime(const char *aname)
{
	char path[MAXPATHLEN];
	struct data_foreach d;
	struct stat sb;

	snprintf(path, MAXPATHLEN, CHROOT_DIR ARTICLES_DIR
	    "/%s/article", aname);
	if (stat(path, &sb) == -1)
		return 0;
	d.time = sb.st_mtime;
	snprintf(path, MAXPATHLEN, CHROOT_DIR ARTICLES_DIR
		    "/%s/comments", aname);
	if (stat(path, &sb) != -1) {
		if (sb.st_mtime > d.time)
			d.time = sb.st_mtime;
		foreach_comment(aname, do_get_mtime_comments, &d.time);
	}
	d.name = aname;
	foreach_tag(do_get_mtime_tags, &d);
	return d.time;
}

#endif /* ENABLE_STATIC */

/*
 *
 */

int
read_article(const char *aname, article_cb cb, char *atitle, size_t atitle_len)
{
	FILE *fin;
	char path[MAXPATHLEN], title[BUFSIZ];
	struct tm tm;

	memset(&tm, 0, sizeof(struct tm));
	if (strptime(aname, FILE_FORMAT, &tm) == NULL)
		return -1;
	mktime(&tm);
#ifdef ENABLE_STATIC
	if (from_cmd)
		snprintf(path, MAXPATHLEN, CHROOT_DIR ARTICLES_DIR
		    "/%s/article", aname);
	else
#endif /* ENABLE_STATIC */
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
#ifdef ENABLE_COMMENTS
		cb(aname, title, &tm, fin, read_comments(aname, NULL));
#else
		cb(aname, title, &tm, fin, 0);
#endif /* ENABLE_COMMENTS */
	fclose(fin);
	return 0;
}

/*
 * Read every articles of ARTICLES_DIR or of TAGS_DIR.
 */

static int
do_read_articles(const char *aname, void *data)
{
	struct data_foreach *d = data;
	long nb;
	extern long offset;

	nb = offset*NB_ARTICLES;
	if (d->nb > nb+NB_ARTICLES)
		return 0;
	else if (d->nb >= nb)
		read_article(aname, d->a_cb, NULL, 0);
	++d->nb;
	return 1;
}

void
read_articles(article_cb cb)
{
	struct data_foreach d;

	d.nb = 0;
	d.a_cb = cb;
	foreach_article(do_read_articles, &d);
}

/*
 * Get the number of the page of an article in ARTICLES_DIR or in TAGS_DIR.
 * If no article is specified, then it will get the total number of articles.
 */

static int
do_get_page(const char *aname, void *data)
{
	struct data_foreach *d = data;

	if (d->name != NULL
	    && strcmp(aname, d->name) == 0)
		return 0;
	++d->nb;
	return 1;
}

long
get_page(const char *aname)
{
	struct data_foreach d;

	d.name = aname;
	d.nb = 0;
	if (!foreach_article(do_get_page, &d) || aname == NULL)
		return d.nb/NB_ARTICLES;
	return -1;
}

/*
 *
 */

static int
do2_read_tags(const char *aname, void *data)
{
	uint *nb = data;

	++*nb;
	return 1;
}

static int
do_read_tags(const char *tname, void *data)
{
	tag_cb *cb = data;
	const char *old_tag;
	extern const char *tag;
	uint nb;

	nb = 0;
	old_tag = tag;
	tag = tname;
	foreach_article(do2_read_tags, &nb);
	tag = old_tag;
	cb(tname, nb);
	return 1;
}

void
read_tags(tag_cb cb)
{
	foreach_tag(do_read_tags, cb);
}
