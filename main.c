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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

void render_error(char *);
void render_article(char *);
void render_tags(char *);
void render_page(page_cb, char *);
void render_rss(void);

#ifdef ENABLE_GZIP
#include <zlib.h>
gzFile	 gz;
#endif /* ENABLE_GZIP */

#if defined(ENABLE_COMMENTS) && defined(ENABLE_POST_COMMENT)
void post_comment(char *);
struct cform	comment_form;
#endif /* ENABLE_POST_COMMENT */

#ifdef ENABLE_STATIC
void generate_static(void);
void update_static(void);
void update_static_article(char *);
int from_cmd, follow_url, generating_static;
#endif /* ENABLE_STATIC */

FILE	*hout;
char	*tag;
long	 offset;

static char *
get_params(void)
{
	char *env;

	if ((env = getenv("PATH_INFO")) == NULL)
		return NULL;
	while (*env == '/')
		++env;
	if (*env == '\0')
		return NULL;
	return env;
}

static void
parse_query(void)
{
	char *q, *s;
	long qoffset;
	const char *errstr;

	q = getenv("QUERY_STRING");
	for (; q != NULL && *q != '\0'; q = s) {
		if ((s = strchr(q, '&')) != NULL)
			*s++ = '\0';	
		if (strncmp(q, "p=", 2) == 0) {
			q += 2;
			qoffset = strtonum(q, 0, LONG_MAX, &errstr);
			if (errstr == NULL)
				offset = qoffset;
		}
	}
}

#ifdef ENABLE_GZIP

static void
enable_gzip(void)
{
	char *env;

	if ((env = getenv("HTTP_ACCEPT_ENCODING")) == NULL)
		return;
	if (strstr(env, "gzip") == NULL)
		return;
	if ((gz = gzdopen(fileno(stdout), "wb9")) == NULL)
		if (errno != 0)
			warn("gzdopen");
		else
			warnx("gzdopen");
}

#endif /* ENABLE_GZIP */

void
redirect(char *aname)
{
	fputs("Status: 302\r\nLocation: ", stdout);
#ifdef ENABLE_STATIC
	fputs(BASE_URL, stdout);
	if (aname != NULL) {
		fputs(aname, stdout);
		fputs(".html", stdout);
	}
#else
	fputs(BIN_URL, stdout);
	if (aname != NULL) {
		fputc('/', stdout);
		fputs(aname, stdout);
	}
#endif /* ENABLE_STATIC */
	fputs("\r\n\r\n", stdout);
	fflush(stdout);
}

int
main(int argc, char **argv)
{
	char *p;

#ifdef ENABLE_GZIP
	gz = NULL;
	enable_gzip();
#endif /* ENABLE_GZIP */
	hout = stdout;
	tag = NULL;
	offset = 0;
#if defined(ENABLE_COMMENTS) && defined(ENABLE_POST_COMMENT)
	memset(&comment_form, 0, sizeof(struct cform));
#endif /* POST_COMMENT */
	parse_query();

#ifdef ENABLE_STATIC
	generating_static = follow_url = from_cmd = 0;
	if ((from_cmd = (getenv("GEN_STATIC") != NULL))) {
		generate_static();
	} else if ((from_cmd = ((p = getenv("UP_STATIC")) != NULL))) {
		if (p != NULL
		    && (strncmp(p, "20", 2) == 0 || strncmp(p, "19", 2) == 0))
			update_static_article(p);
		else
			update_static();
#if defined(ENABLE_COMMENTS) && defined(ENABLE_POST_COMMENT)
	} else if ((p = get_params()) != NULL
	    && (strncmp(p, "20", 2) == 0 || strncmp(p, "19", 2) == 0)
	    && strcmp(getenv("REQUEST_METHOD"), "POST") == 0) {
		post_comment(p);
#endif /* POST_COMMENT */
	} else
		redirect(NULL);

#else

	if ((p = get_params()) != NULL) {
		if (strncmp(p, "20", 2) == 0 || strncmp(p, "19", 2) == 0) {
#if defined(ENABLE_COMMENTS) && defined(ENABLE_POST_COMMENT)
			if (strcmp(getenv("REQUEST_METHOD"), "POST") == 0)
				post_comment(p);
			else
#endif /* POST_COMMENT */
				render_page(render_article, p);
		} else if (strncmp(p, "tag/", 4) == 0) {
			if (p[4] != '\0')
				tag = p+4;
			render_page(render_article, NULL);
		} else if (strcmp(p, "tags") == 0) {
			render_page(render_tags, NULL);
		} else if (strncmp(p, "rss", 3) == 0) {
			if (p[3] == '/' && p[4] != '\0')
				tag = p+4;
			render_rss();
		} else
			render_page(render_error, ERR_PAGE_UNKNOWN);
	} else
		render_page(render_article, NULL);
#endif /* ENABLE_STATIC */

#ifdef ENABLE_GZIP
	if (gz != NULL)
		gzclose(gz);
	else
#endif /* ENABLE_GZIP */
		fflush(hout);
	return 0;
}
