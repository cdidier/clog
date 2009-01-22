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
#include "output.h"

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
hput_url(int type, const char *name, ulong page)
{
#ifdef ENABLE_STATIC
	extern int follow_url;
#endif /* ENABLE_STATIC */

	if (type == PAGE_UNKNOWN)
		hput_escaped(name);
	else {
#ifdef ENABLE_STATIC
		hputs(BASE_URL);
#else
		hputs(BIN_URL);
		if (type != PAGE_INDEX && name != NULL)
			hputc('/');
#endif /* ENABLE_STATIC */
		switch (type) {
		case PAGE_INDEX:
			if (name != NULL) {
				hputs("tag");
#ifdef ENABLE_STATIC
				hputc('_');
#else
				hputc('/');
#endif /* ENABLE_STATIC */
				hputs(name);
			}
			if (page > 0) {
#ifdef ENABLE_STATIC
				if (name == NULL)
					hputs("index");
				hputc('_');
				hputd(page);
#else
				hputs("?p=");
				hputd(page);
#endif /* ENABLE_STATIC */
			}
#ifdef ENABLE_STATIC
			if (page > 0 || name != NULL)
				hputs(".html");
			if (follow_url)
				add_static_tag(name, page);
#endif /* ENABLE_STATIC */
			break;
		case PAGE_ARTICLE:
			hputs(name);
#ifdef ENABLE_STATIC
			hputs(".html");
			if (follow_url)
				add_static_article(name);
#endif /* ENABLE_STATIC */
			break;
		case PAGE_RSS:
			hputs("rss");
			if (name != NULL) {
#ifdef ENABLE_STATIC
				hputc('_');
#else
				hputc('/');
#endif /* ENABLE_STATIC */
				hputs(name);
			}
#ifdef ENABLE_STATIC
			hputs(".xml");
#endif /* ENABLE_STATIC */
			break;
		case PAGE_TAG_CLOUD:
			hputs("tags");
#ifdef ENABLE_STATIC
			hputs(".html");
#endif /* ENABLE_STATIC */
			break;
		}
	}
}

void
hput_pagelink(int type, const char *name, ulong page, const char *text)
{
	hputs("<a href=\"");
	hput_url(type, name, page);
	hputs("\">");
	hput_escaped(text);
	hputs("</a>");
}

static int
hput_generic_markers(const char *m)
{ 
	extern struct page globp;

	if (strcmp(m, "BASE_URL") == 0)
		hput_url(PAGE_INDEX, NULL, 0);
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
		if (globp.type == PAGE_INDEX && globp.i.tag != NULL) {
			hputs(" - tag:");
			hputs(globp.i.tag);
		}
	} else if (strcmp(m, "RSS_URL") == 0) {
		if (globp.type == PAGE_INDEX)
			hput_url(PAGE_RSS, globp.i.tag, 0);
		else
			hput_url(PAGE_RSS, NULL, 0);
	} else
		return 0;
	return 1;
}

void
parse_template(const char *file, parse_markers_cb cb, void *data)
{
	char path[MAXPATHLEN], buf[BUFSIZ], *a, *b;
	FILE *fin;

#ifdef ENABLE_STATIC
	if (from_cmd)
		snprintf(path, MAXPATHLEN, CHROOT_DIR TEMPLATES_DIR "/%s",
		   file);
	else
#endif /* ENABLE_STATIC */
		snprintf(path, MAXPATHLEN, TEMPLATES_DIR "/%s", file);
	if ((fin = fopen(path, "r")) == NULL) {
		 warn("fopen: %s", path);
		 return;
	}
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
