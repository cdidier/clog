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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"

#ifdef ENABLE_STATIC

void render_article(char *);
void render_tags(char *);
void render_page(page_cb, char *);
void render_rss(void);

struct vtag {
	char	*tname;
	long	 page;
	SLIST_ENTRY(vtag)	next;
};

struct varticle {
	char	*aname;
	SLIST_ENTRY(varticle)	next;
};

static SLIST_HEAD(, vtag) tovisit_tags;
static SLIST_HEAD(, vtag) visited_tags;
static SLIST_HEAD(, varticle) tovisit_articles;
static SLIST_HEAD(, varticle) visited_articles;
extern int gen, from_cmd;

void
add_static_tag(char *tname, long page)
{
	struct vtag *vt, *end;

	if (!gen)
		return;
	SLIST_FOREACH(vt, &tovisit_tags, next) {
		if ((vt->tname == NULL && tname == NULL)
		    || (vt->tname != NULL && tname != NULL
		    && strcmp(vt->tname, tname) == 0))
			if (vt->page == page)
				return;
	}
	SLIST_FOREACH(vt, &visited_tags, next) {
		if ((vt->tname == NULL && tname == NULL)
		    || (vt->tname != NULL && tname != NULL
		    && strcmp(vt->tname, tname) == 0))
			if (vt->page == page)
				return;
	}
	if ((vt = malloc(sizeof(struct vtag))) == NULL)
		err(1, "malloc");
	if (tname == NULL)
		vt->tname = NULL;
	else if ((vt->tname = strdup(tname)) == NULL)
		err(1, "strdup");
	vt->page = page;
	if ((end = SLIST_FIRST(&tovisit_tags)) != NULL)
		while(SLIST_NEXT(end, next) != NULL)
			end = SLIST_NEXT(end, next);
	if (end == NULL)
		SLIST_INSERT_HEAD(&tovisit_tags, vt, next);
	else
		SLIST_INSERT_AFTER(end, vt, next);
}

void
add_static_article(char *aname)
{
	struct varticle *va;

	if (!gen)
		return;
	SLIST_FOREACH(va, &tovisit_articles, next) {
		if ((va->aname == NULL && aname == NULL)
		    || (va->aname != NULL && aname != NULL
		    && strcmp(va->aname, aname) == 0))
			return;
	}
	SLIST_FOREACH(va, &visited_articles, next) {
		if ((va->aname == NULL && aname == NULL)
		    || (va->aname != NULL && aname != NULL
		    && strcmp(va->aname, aname) == 0))
			return;
	}
	if ((va = malloc(sizeof(struct varticle))) == NULL)
		err(1, "malloc");
	if ((va->aname = strdup(aname)) == NULL)
		err(1, "strdup");
	SLIST_INSERT_HEAD(&tovisit_articles, va, next);
}

void
gen_article(char *aname)
{
	char path[MAXPATHLEN];
	mode_t old_mask;
	FILE *old_hout;
	extern FILE *hout;
	extern char *tag;
	extern long offset;

	if (from_cmd)
		snprintf(path, MAXPATHLEN, CHROOT_DIR BASE_DIR
		    "/%s.html", aname);
	else
		snprintf(path, MAXPATHLEN, BASE_DIR"/%s.html", aname);
	old_mask = umask(0002);
	old_hout = hout;
	if ((hout = fopen(path, "w")) == NULL)
		err(1, "fopen: %s", path);
	umask(old_mask);
	tag = NULL;
	offset = 0;
	render_page(render_article, aname);
	fclose(hout);
	hout = old_hout;
}

void
gen_rss(char *tname)
{
	char path[MAXPATHLEN];
	mode_t old_mask;
	FILE *old_hout;
	extern FILE *hout;
	extern char *tag;

	snprintf(path, MAXPATHLEN, CHROOT_DIR BASE_DIR"/rss%s%s.xml",
	    tname  != NULL ? "_" : "", tname  != NULL ? tname : "");
	old_mask = umask(0002);
	old_hout = hout;
	if ((hout = fopen(path, "w")) == NULL)
		err(1, "fopen: %s", path);
	umask(old_mask);
	tag = tname;
	render_rss();
	fclose(hout);
	hout = old_hout;
}

static void
gen_index(char *tname, long page)
{
	char path[MAXPATHLEN], pagenum[BUFSIZ];
	mode_t old_mask;
	FILE *old_hout;
	extern FILE *hout;
	extern char *tag;
	extern long offset;

	snprintf(pagenum, BUFSIZ, "%ld", page);
	if (from_cmd)
		snprintf(path, MAXPATHLEN, CHROOT_DIR BASE_DIR"/%s%s%s%s.html",
		    	tname  != NULL ? "tag_" : "index",
			tname  != NULL ? tname : "",
		    	page > 0 ? "_" : "", page > 0 ? pagenum : "");
	else
		snprintf(path, MAXPATHLEN, BASE_DIR"/%s%s%s%s.html",
		    	tname  != NULL ? "tag_" : "index",
			tname  != NULL ? tname : "",
		    	page > 0 ? "_" : "", page > 0 ? pagenum : "");
	old_mask = umask(0002);
	old_hout = hout;
	if ((hout = fopen(path, "w")) == NULL)
		 err(1, "fopen: %s", path);
	umask(old_mask);
	tag = tname;
	offset = page;
	render_page(render_article, NULL);
	fclose(hout);
	hout = old_hout;
	if (page == 0)
		gen_rss(tname);
}

static void
gen_tags(void)
{
	char path[MAXPATHLEN];
	mode_t old_mask;
	FILE *old_hout;
	extern FILE *hout;
	extern char *tag;

	strlcpy(path, CHROOT_DIR BASE_DIR"/tags.html", MAXPATHLEN);
	old_mask = umask(0002);
	old_hout = hout;
	if ((hout = fopen(path, "w")) == NULL)
		err(1, "fopen: %s", path);
	umask(old_mask);
	tag = NULL;
	render_page(render_tags, NULL);
	fclose(hout);
	hout = old_hout;
}

void
update_article(char *aname)
{
}

void
update_static(void)
{
}

void
generate_static(void)
{
	struct vtag *vt;
	struct varticle *va;

	gen = 1;
	SLIST_INIT(&tovisit_tags);
	SLIST_INIT(&visited_tags);
	SLIST_INIT(&tovisit_articles);
	SLIST_INIT(&visited_articles);
	add_static_tag(NULL, 0); /* Starting point: index.html */
	while((vt = SLIST_FIRST(&tovisit_tags)) != NULL) {
		gen_index(vt->tname, vt->page);
		SLIST_REMOVE_HEAD(&tovisit_tags, next);
		SLIST_INSERT_HEAD(&visited_tags, vt, next);
	}
	while((va = SLIST_FIRST(&tovisit_articles)) != NULL) {
		gen_article(va->aname);
		SLIST_REMOVE_HEAD(&tovisit_articles, next);
		SLIST_INSERT_HEAD(&visited_articles, va, next);
	}
	gen_tags();
}

#endif /* ENABLE_STATIC */
