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

#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "output.h"
#include "articles.h"
#include "comments.h"

void render_page_article(struct article *);
void render_page_tag(struct tag *);
void render_page_tags(void);
void render_rss(struct tag *);
void sanitize_input(char *);
void generate_static(const char *cmd);

enum STATUS	 status;
char		*error_str;
struct query	*query_get, *query_post;

#ifdef DEFAULT_STATIC
static void
handle_static_url(void)
{
	char *q, *s;
	unsigned long p;
	const char *errstr;

	status |= STATUS_STATIC;
	if ((q = get_query_param(query_get, "page")) != NULL && *q != '\0') {
		if (strcmp(q, "tags") == 0) {
			document_begin_redirection();
			hput_url("tags");
			document_end_redirection();
		} else if (strcmp(q, "rss") == 0) {
			if ((q = get_query_param(query_get, "tag")) != NULL) {
				if (*q != '\0')
					sanitize_input(q);
				else
					q = NULL;
			}
			document_begin_redirection();
			hput_url("rss", q);
			document_end_redirection();
		} else
			document_not_found();
	} else if ((q = get_query_param(query_get, "article")) != NULL) {
		if (!is_article_name(q, strlen(q))) {
			document_not_found();
			return;
		}
		sanitize_input(q);
		if (status & STATUS_POST) {
			if (post_comment(q) == -1) {
				if (read_article(q, render_page_article) == -1)
					document_not_found();
				return;
			}
			generate_static(q);
		}
		document_begin_redirection();
		hput_url("article", q);
		document_end_redirection();
	} else {
		/* extract the page */
		p = 0;
		if ((s = get_query_param(query_get, "p")) != NULL) {
			p = strtonum(s, 0, LONG_MAX, &errstr);
			if (errstr != NULL) {
				document_not_found();
				return;
			}
		}
		/* extract the tag */
		if ((q = get_query_param(query_get, "tag")) != NULL) {
			if (*q != '\0')
				sanitize_input(q);
			else
				q = NULL;
		}
		document_begin_redirection();
		hput_url("tag", q, p, NB_ARTICLES);
		document_end_redirection();
	}
}
#endif

static void
handle_url(void)
{
	char *q, *s;
	unsigned long p, n;
	const char *errstr;

	if ((q = get_query_param(query_get, "page")) != NULL && *q != '\0') {
		if (strcmp(q, "tags") == 0)
			render_page_tags();
		else if (strcmp(q, "rss") == 0) {
			if ((q = get_query_param(query_get, "tag")) != NULL) {
				if (*q != '\0')
					sanitize_input(q);
				else
					q = NULL;
			}
			if (read_tag(q, 0, NB_ARTICLES, render_rss) == -1)
				document_not_found();
		} else
			document_not_found();
	} else if ((q = get_query_param(query_get, "article")) != NULL) {
		if (!is_article_name(q, strlen(q))) {
			document_not_found();
			return;
		}
		sanitize_input(q);
		if (status & STATUS_POST && post_comment(q) != -1
		    && !(status & STATUS_FROMCMD)) {
			document_begin_redirection();
			hput_url("article", q);
			document_end_redirection();
		} else if (read_article(q, render_page_article) == -1)
			document_not_found();
	} else {
		/* extract the page */
		p = 0;
		if ((s = get_query_param(query_get, "p")) != NULL) {
			p = strtonum(s, 0, LONG_MAX, &errstr);
			if (errstr != NULL) {
				document_not_found();
				return;
			}
		}
		/* extract the number of article per page */
		n = NB_ARTICLES;
		if ((s = get_query_param(query_get, "n")) != NULL) {
			n  = strtonum(s, 0, LONG_MAX, &errstr);
			if (errstr != NULL || n == 0)
				n = NB_ARTICLES;
		}
		/* extract the tag */
		if ((q = get_query_param(query_get, "tag")) != NULL) {
			if (*q != '\0')
				sanitize_input(q);
			else
				q = NULL;
		}
		if (read_tag(q, p, n, render_page_tag) == -1)
			document_not_found();
	}
}

static void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-h] [-q query] [-s [page]]\n"
	    "\t -h display this help.\n"
	    "\t -q query a page (everything after '?' in an URL).\n"
	    "\t -s if no argument is given, the links will point to the static files.\n"
	    "\t    if an argument is given, static files of the page will be generated.\n",
	    __progname);
	exit(1);
}

int
main(int argc, char **argv)
{
	char ch;
	char *env;

	status = STATUS_NONE;
	if (getenv("SERVER_NAME") == NULL)
		status |= STATUS_FROMCMD;
	if ((env = getenv("REQUEST_METHOD")) != NULL
	    && strcmp(env, "POST") == 0)
		status |= STATUS_POST;
	error_str = NULL;
	if (status & STATUS_FROMCMD) {
		while ((ch = getopt(argc, argv, "hq:s")) != -1)
			switch (ch) {
			case 'q':
				if (setenv("QUERY_STRING", optarg, 1) == -1)
					err(1, "setenv");
				break;
			case 's':
				status |= STATUS_STATIC;
				/* handle the optionnal parameter */
				if ((argv[optind]) && (argv[optind][0] != '-')) {
					generate_static(argv[optind++]);
					goto out;
				}
				break;
			default:
				usage();
			}
	}
	query_get = tokenize_query(getenv("QUERY_STRING"));
	query_post = NULL;
#ifdef DEFAULT_STATIC
	if (status & STATUS_FROMCMD)
		handle_url();
	else
		handle_static_url();
#else
	handle_url();
#endif
out:	free_query(query_get);
	if (status & STATUS_POST)
		free_query(query_post);
	return 0;
}
