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
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "output.h"
#include "articles.h"

void render_page_article(struct article *);
void render_page_tag(struct tag *);
void render_page_tags(void);
void render_rss(struct tag *);

static void
write_article_file(struct article *a)
{
	char path[MAXPATHLEN];
	struct stat sb;
	extern FILE *hout;
	extern enum STATUS status;

	snprintf(path, MAXPATHLEN, "%s" BASE_DIR "/%s.html",
	    status & STATUS_FROMCMD ? CHROOT_DIR : "", a->name);
	fprintf(stderr, "Writing %s...\n", path);
	if ((hout = fopen(path, "w")) == NULL)
		warn("fopen: %s", path);
	else {
		render_page_article(a);
		fclose(hout);
		if (stat(path, &sb) != -1 && sb.st_size == 0)
			unlink(path);
	}
}

static void
write_tag_file(const char *tag, unsigned long page)
{
	char path[MAXPATHLEN];
	struct stat sb;
	char page_str[20];
	extern FILE *hout;
	extern enum STATUS status;

	snprintf(page_str, sizeof(page_str), "%lu", page);
	snprintf(path, MAXPATHLEN, "%s" BASE_DIR "/index%s%s%s%s.html",
	    status & STATUS_FROMCMD ? CHROOT_DIR : "",
	    tag != NULL ? "_" : "", tag != NULL ? tag : "",
	    page != 0 ? "-" : "", page != 0 ? page_str : "");
	fprintf(stderr, "Writing %s...\n", path);
	if ((hout = fopen(path, "w")) == NULL)
		warn("fopen: %s", path);
	else {
		read_tag(tag, page, NB_ARTICLES, render_page_tag);
		fclose(hout);
		if (stat(path, &sb) != -1 && sb.st_size == 0)
			unlink(path);
	}
}

static void
write_tags_file(void)
{
	char path[MAXPATHLEN];
	struct stat sb;
	extern FILE *hout;
	extern enum STATUS status;
	
	snprintf(path, MAXPATHLEN, "%s" BASE_DIR "/tags.html",
	    status & STATUS_FROMCMD ? CHROOT_DIR : "");
	fprintf(stderr, "Writing %s...\n", path);
	if ((hout = fopen(path, "w")) == NULL)
		warn("fopen: %s", path);
	else {
		render_page_tags();
		fclose(hout);
		if (stat(path, &sb) != -1 && sb.st_size == 0)
			unlink(path);
	}
}

static void
write_rss_file(const char *tag)
{
	char path[MAXPATHLEN];
	struct stat sb;
	extern FILE *hout;
	extern enum STATUS status;

	snprintf(path, MAXPATHLEN, "%s" BASE_DIR "/rss%s%s.xml",
	    status & STATUS_FROMCMD ? CHROOT_DIR : "",
	    tag != NULL ? "_" : "", tag != NULL ? tag : "");
	fprintf(stderr, "Writing %s...\n", path);
	if ((hout = fopen(path, "w")) == NULL)
		warn("fopen: %s", path);
	else {
		read_tag(tag, 0, NB_ARTICLES, render_rss);
		fclose(hout);
		if (stat(path, &sb) != -1 && sb.st_size == 0)
			unlink(path);
	}
}

static void
generate_article(struct article *a)
{
	struct article_tag *at;
	unsigned long page;

	write_article_file(a);
	SLIST_FOREACH(at, &a->tags, next) {
		page = at->number/NB_ARTICLES
		    + (at->number%NB_ARTICLES != 0 ? 1 : 0) - 1;
		write_tag_file(at->name, page);
		if (page == 0)
			write_rss_file(at->name);
	}
}

static void
generate_tags(void)
{
	SLIST_HEAD(, article_tag) list;
	struct article_tag tmp, *at;
	unsigned long nb_articles, pages, p;

	SLIST_FIRST(&list) = get_article_tags(NULL);
	tmp.name = NULL;
	SLIST_INSERT_HEAD(&list, &tmp, next);
	SLIST_FOREACH(at, &list, next) {
		nb_articles = read_articles(at->name, 0, 0, NULL);
		pages = nb_articles/NB_ARTICLES
		    + (nb_articles%NB_ARTICLES != 0 ? 1 : 0);
		for (p = 0; p < pages; ++p)
			write_tag_file(at->name, p);
	}
	SLIST_REMOVE_HEAD(&list, next);
	while (!SLIST_EMPTY(&list)) {
		at = SLIST_FIRST(&list);
		SLIST_REMOVE_HEAD(&list, next);
		free(at->name);
		free(at);
	}
	write_tags_file();
}

static void
generate_rss(void)
{
	SLIST_HEAD(, article_tag) list;
	struct article_tag tmp, *at;

	SLIST_FIRST(&list) = get_article_tags(NULL);
	tmp.name = NULL;
	SLIST_INSERT_HEAD(&list, &tmp, next);
	SLIST_FOREACH(at, &list, next)
		write_rss_file(at->name);
	SLIST_REMOVE_HEAD(&list, next);
	while (!SLIST_EMPTY(&list)) {
		at = SLIST_FIRST(&list);
		SLIST_REMOVE_HEAD(&list, next);
		free(at->name);
		free(at);
	}
}

void
generate_static(const char *cmd)
{
	FILE *old_f;
	mode_t old_mask;
	extern FILE *hout;
	extern enum STATUS status;

	status |= STATUS_STATIC;
	old_f = hout;
	old_mask = umask(0002);
	if (strcmp(cmd, "all") == 0) {
		generate_tags();
		read_articles(NULL, 0, 0, write_article_file);
		generate_rss();
	} else if (strcmp(cmd, "tags") == 0)
		generate_tags();
	else if (strcmp(cmd, "rss") == 0)
		generate_rss();
	else if (strcmp(cmd, "articles") == 0)
		read_articles(NULL, 0, 0, write_article_file);
	else if (is_article_name(cmd, strlen(cmd))) {
		read_article(cmd, generate_article);
	} else
		document_not_found();
	status ^= STATUS_STATIC;
	hout = old_f;
	umask(old_mask);
}
