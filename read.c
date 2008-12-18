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

struct data_read_comments {
	comment_cb	*cb;
	ulong		 nb;
};

int
do_read_comments(const char *aname, const char *cname, void *data)
{
	struct data_read_comments *d = data;

	if (d->cb != NULL)
		read_comment(aname, cname, d->cb);
	++d->nb;
	return 1;
}

ulong
read_comments(const char *aname, comment_cb cb)
{
	struct data_read_comments d;

	d.cb = cb;
	d.nb = 0;
	foreach_comment(aname, do_read_comments, &d);
	return d.nb;
}

#endif /* ENABLE_COMMENTS */

/*
 *
 */

struct data_read_article_tags {
	article_tag_cb	*cb;
	const char	*aname;
	ulong		 nb;
};

static int
do2_read_article_tags(const char *aname, void *data)
{
	struct data_read_article_tags *d = data;
	extern const char *tag;

	if (strcmp(d->aname, aname) == 0) {
		++d->nb;
		d->cb(tag);
	}
	return 1;
}

static int
do_read_article_tags(const char *tname, void *data)
{
	struct data_read_article_tags *d = data;
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
	struct data_read_article_tags d;

	d.cb = cb;
	d.aname = aname;
	d.nb = 0;
	foreach_tag(do_read_article_tags, &d);
	return d.nb;
}

#ifdef ENABLE_STATIC

/*
 *
 */

int
do_read_article_mtime(const char *aname, const char *cname, void *data)
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
read_article_mtime(const char *aname)
{
	char path[MAXPATHLEN];
	struct stat sb;
	time_t mtime;

	snprintf(path, MAXPATHLEN, CHROOT_DIR ARTICLES_DIR
	    "/%s/article", aname);
	if (stat(path, &sb) == -1)
		return 0;
	mtime = sb.st_mtime;
	snprintf(path, MAXPATHLEN, CHROOT_DIR ARTICLES_DIR
		    "/%s/comments", aname);
	if (stat(path, &sb) != -1) {
		if (sb.st_mtime > mtime)
			mtime = sb.st_mtime;
		foreach_comment(aname, do_read_article_mtime, &mtime);
	}
	return mtime;
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

struct data_read_articles {
	article_cb	*cb;
	long		 nb;
};

static int
do_read_articles(const char *aname, void *data)
{
	struct data_read_articles *d = data;
	long nb;
	extern long offset;

	nb = offset*NB_ARTICLES;
	if (d->nb > nb+NB_ARTICLES)
		return 0;
	else if (d->nb >= nb)
		read_article(aname, d->cb, NULL, 0);
	++d->nb;
	return 1;
}

void
read_articles(article_cb cb)
{
	struct data_read_articles d;

	d.cb = cb;
	d.nb = 0;
	foreach_article(do_read_articles, &d);
}

/*
 * Get the number of the page of an article in ARTICLES_DIR or in TAGS_DIR.
 * If no article is specified, then it will get the total number of articles.
 */

struct data_read_num_page {
	const char	*aname;
	long		nb;
};

static int
do_read_num_page(const char *aname, void *data)
{
	struct data_read_num_page *d = data;

	if (d->aname != NULL
	    && strcmp(aname, d->aname) == 0)
		return 0;
	++d->nb;
	return 1;
}

long
read_num_page(const char *aname)
{
	struct data_read_num_page d;

	d.aname = aname;
	d.nb = 0;
	if (!foreach_article(do_read_num_page, &d) || aname == NULL)
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
