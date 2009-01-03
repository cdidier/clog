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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>

#ifndef LINUX
#include <sha1.h>
#else
#include "openbsd-compat/sha1.h"
#endif

#include "common.h"
#include "render_tools.h"

#ifdef ENABLE_GZIP
#include <zlib.h>
extern gzFile	 gz;
#endif /* ENABLE_GZIP */

#ifdef ENABLE_STATIC
void	add_static_tag(const char *, long);
void	add_static_article(const char *);
extern int from_cmd, generating_static;
#endif /* ENABLE_STATIC */

#define MARKER_TAG "%%"

extern FILE	*hout;

FILE *
open_template(const char *file)
{
	FILE *fin;
	char path[MAXPATHLEN];

#ifdef ENABLE_STATIC
	if (from_cmd)
		snprintf(path, MAXPATHLEN, CHROOT_DIR TEMPLATES_DIR "/%s",
		   file);
	else
#endif /* ENABLE_STATIC */
		snprintf(path, MAXPATHLEN, TEMPLATES_DIR "/%s", file);
	if ((fin = fopen(path, "r")) == NULL)
		 warn("fopen: %s", path);
	return fin;
}

void
hputc(const char c)
{
#ifdef ENABLE_GZIP
#ifdef ENABLE_STATIC
	if (gz != NULL && !generating_static)
#else
	if (gz != NULL)
#endif /* ENABLE_STATIC */
		gzputc(gz, c);
	else
#endif /* ENABLE_GZIP */
		fputc(c, hout);
}

void
hputs(const char *s)
{
#ifdef ENABLE_GZIP
#ifdef ENABLE_STATIC
	if (gz != NULL && !generating_static)
#else
	if (gz != NULL)
#endif /* ENABLE_STATIC */
		gzputs(gz, s);
	else
#endif /* ENABLE_GZIP */
		fputs(s, hout);
}

void
hputd(const long long l)
{
#ifdef ENABLE_GZIP
#ifdef ENABLE_STATIC
	if (gz != NULL && !generating_static)
#else
	if (gz != NULL)
#endif /* ENABLE_STATIC */
		gzprintf(gz, "%lld", l);
	else
#endif /* ENABLE_GZIP */
		fprintf(hout, "%lld", l);
}

void
hput_escaped(const char *s)
{
	char *a, *b;
	char c;

	for (a = (char *)s; (b = strpbrk(a, "<>'\"&\n")) != NULL; a = b+1) {
		c = *b;
		*b = '\0';
		hputs(a);
		*b = c;
		switch(c) {
		case '<':
			hputs("&lt;");
			break;
		case '>':
			hputs("&gt;");
			break;
		case '\'':
			hputs("&#039;");
			break;
		case '"':
			hputs("&quot;");
			break;
		case '&':
			hputs("&amp;");
			break;
		case '\n':
			hputs("<br>\n");
			break;
		case '\r':
			break;
		default:
			hputc(c);
		}
	}
	hputs(a);
}

void
hput_url(const char *first_level, const char *second_level)
{
#ifdef ENABLE_STATIC
	extern int follow_url;

	hputs(BASE_URL);
#else
	hputs(BIN_URL);
#endif /* ENABLE_STATIC */
	if (first_level != NULL) {
#ifndef ENABLE_STATIC
	hputc('/');
#endif /*! ENABLE_STATIC */
		hputs(first_level);
		if (second_level != NULL) {
#ifdef ENABLE_STATIC
			hputc('_');
#else
			hputc('/');
#endif /* ENABLE_STATIC */
			hputs(second_level);
#ifdef ENABLE_STATIC
			if (strcmp(first_level, "rss") == 0)
				hputs(".xml");
			else
				hputs(".html");
		} else if (strcmp(first_level, "rss") == 0)
			hputs(".xml");
		else
			hputs(".html");
#else
		}
#endif /* ENABLE_STATIC */
	}
#ifdef ENABLE_STATIC
	if (follow_url && first_level != NULL && second_level == NULL
	    && (strncmp(first_level, "20", 2) == 0
	    || strncmp(first_level, "19", 2) == 0))
		add_static_article(first_level);
#endif /* ENABLE_STATIC */
}

void
hput_pagelink(const char *tname, long page, const char *text)
{
#ifdef ENABLE_STATIC
	extern int follow_url;
#endif /* ENABLE_STATIC */

	hputs("<a href=\"");
#ifdef ENABLE_STATIC
	hput_url(NULL, NULL);
	if (page > 0) {
		if (tname != NULL) {
			hputs("tag_");
			hputs(tname);
		} else
			hputs("index");
		hputc('_');
		hputd(page);
		hputs(".html");
	} else if (tname != NULL) {
		hputs("tag_");
		hputs(tname);
		hputs(".html");
	}
#else
	if (tname != NULL)
		hput_url("tag", tname);
	else
		hput_url(NULL, NULL);
	if (page > 0) {
		hputs("?p=");
		hputd(page);
	}
#endif /* ENABLE_STATIC */
	hputs("\">");
	hputs(text);
	hputs("</a>");
#ifdef ENABLE_STATIC
	if (follow_url)
		add_static_tag(tname, page);
#endif /* ENABLE_STATIC */
}

static int
hput_generic_markers(const char *m)
{ 
	extern const char *tag;

	if (strcmp(m, "BASE_URL") == 0)
		hput_url(NULL, NULL);
	else if (strcmp(m, "SITE_NAME") == 0)
		hputs(SITE_NAME);
	else if (strcmp(m, "DESCRIPTION") == 0)
		hputs(DESCRIPTION);
	else if (strcmp(m, "COPYRIGHT") == 0)
		hputs(COPYRIGHT);
	else if (strcmp(m, "CHARSET") == 0)
		hputs(CHARSET);
	else if (strcmp(m, "RSS_TITLE") == 0) {
		hputs(SITE_NAME);
		hputs(" RSS");
		if (tag != NULL) {
			hputs(" - tag:");
			hputs(tag);
		}
	} else if (strcmp(m, "RSS_URL") == 0)
		hput_url("rss", tag);
	else
		return 0;
	return 1;
}

void
parse_markers(FILE *fin, parse_markers_cb cb, void *data)
{
	char buf[BUFSIZ], *a, *b;

	while (fgets(buf, BUFSIZ, fin) != NULL) {
		buf[strcspn(buf, "\n")] = '\0';
		for (a = buf; (b = strstr(a, MARKER_TAG)) != NULL; a = b+2) {
			*b = '\0';
			hputs(a);
			a = b+2;
			if ((b = strstr(a, MARKER_TAG)) != NULL) {
				*b = '\0';
				if (hput_generic_markers(a));
				else
					cb(a, data);
			}
		}
		hputs(a);
		hputc('\n');
	}
}
