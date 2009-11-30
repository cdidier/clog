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

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <zlib.h>

#include "common.h"
#include "output.h"

#define MARKER_TAG "%%"

FILE		*hout = stdout;
static gzFile	 gz = NULL;
extern enum STATUS status;

void
open_output(const char *type)
{
	char *env;

	if (status & STATUS_FROMCMD || status & STATUS_STATIC)
		return;
	if ((env = getenv("HTTP_ACCEPT_ENCODING")) != NULL
	    && strstr(env, "gzip") != NULL) {
		if ((gz = gzdopen(fileno(stdout), "wb9")) == NULL) {
			if (errno != 0)
				warn("gzdopen");
			else
				warnx("gzdopen");
		} else
			fputs("Content-Encoding: gzip\r\n", stdout);
	}
	fprintf(stdout, "Content-type: %s;charset=" CHARSET "\r\n\r\n", type);
	fflush(stdout);
}

void
close_output(void)
{
	if (gz != NULL)
		gzclose(gz);
	else
		fflush(hout);
}

void
document_begin_redirection(void)
{
	fputs("Status: 302\r\nLocation: ", stdout);	
}

void
document_end_redirection(void)
{
	fputs("\r\n\r\n", stdout);
	fflush(stdout);
}

void
document_not_found(void)
{
	if (status & STATUS_STATIC && !(status & STATUS_FROMCMD))
		return;
	else if (status & STATUS_FROMCMD)
		warnx("Document not found.");
	else {
		fputs("Status: 404 Not found\r\n"
		    "Content-type: text/html\r\n\r\n"
		    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
		    "<HTML><HEAD>\n"
		    "<TITLE>404 Not Found</TITLE>\n"
		    "</HEAD><BODY>\n"
		    "<H1>Not Found</H1>\n"
		    "The requested URL ", stdout);
		fputs(getenv("SCRIPT_NAME"), stdout);
		fputc('?', stdout);
		fputs(getenv("QUERY_STRING"), stdout);
		fputs(" was not found on this server.<P>\n"
		    "</BODY></HTML>\n", stdout);
		fflush(stdout);
	}
}

void
hputc(const char c)
{
	if (gz != NULL)
		gzputc(gz, c);
	else
		fputc(c, hout);
}

void
hputs(const char *s)
{
	if (gz != NULL)
		gzputs(gz, s);
	else
		fputs(s, hout);
}

void
hputd(const long long l)
{
	if (gz != NULL)
		gzprintf(gz, "%lld", l);
	else
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
hput_url(char *page, ...)
{
	va_list ap;
	char *s;
	unsigned long p, n;

	hputs(status & STATUS_STATIC ? BASE_URL : BIN_URL);
	va_start(ap, page);
	if (strcmp(page, "article") == 0) {
		s = va_arg(ap, char *);
		if (status & STATUS_STATIC) {
			hputs(s);
			hputs(".html");
		} else {
			 hputs("?article=");
			 hputs(s);
		}
	} else if (strcmp(page, "rss") == 0) {
		s = va_arg(ap, char *);
		if (status & STATUS_STATIC) {
			hputs("rss");
			if (!EMPTYSTRING(s)) {
				hputc('_');
				hputs(s);
			}
			hputs(".xml");
		} else {
			hputs("?page=rss");
			if (!EMPTYSTRING(s)) {
				hputs("&tag=");
				hputs(s);
			}

		}
	} else if (strcmp(page, "tag") == 0) {
		s = va_arg(ap, char *);
		p = va_arg(ap, unsigned long);
		n = va_arg(ap, unsigned long);
		if (status & STATUS_STATIC) {
			hputs("index");
			if (!EMPTYSTRING(s)) {
				hputc('_');
				hputs(s);
			}
			if (p != 0) {
				hputc('-');
				hputd(p);
			}
			hputs(".html");
		} else {
			if (!EMPTYSTRING(s) || p != 0 || n != NB_ARTICLES)
				hputc('?');
			if (!EMPTYSTRING(s)) {
				hputs("tag=");
				hputs(s);
			}
			if (!EMPTYSTRING(s) && p != 0)
				hputc('&');
			if (p != 0) {
				hputs("p=");
				hputd(p);
			}
			if ((!EMPTYSTRING(s) || p != 0) && n != NB_ARTICLES)
				hputc('&');
			if (n != NB_ARTICLES) {
				hputs("n=");
				hputd(n);
			}
		}
	} else if (strcmp(page, "tags") == 0) {
		if (status & STATUS_STATIC)
			hputs("tags.html");
		else
			hputs("?page=tags");
	}
	va_end(ap);
}

static int
hput_generic_markers(const char *m)
{ 
	if (strcmp(m, "BASE_URL") == 0)
		hputs(BASE_URL);
	else if (strcmp(m, "SITE_NAME") == 0)
		hputs(SITE_NAME);
	else if (strcmp(m, "DESCRIPTION") == 0)
		hputs(DESCRIPTION);
	else if (strcmp(m, "CHARSET") == 0)
		hputs(CHARSET);
	else if (strcmp(m, "COPYRIGHT") == 0)
		hputs(COPYRIGHT);
	else
		return 0;
	return 1;
}

void
parse_template(const char *file, markers_cb cb, void *data)
{
	char path[MAXPATHLEN], *buf, *lbuf, *a, *b;
	FILE *f;
	size_t len;

	snprintf(path, MAXPATHLEN, "%s" TEMPLATES_DIR "/%s",
	    status & STATUS_FROMCMD ? CHROOT_DIR : "", file);
	if ((f = fopen(path, "r")) == NULL) {
		 warn("fopen: %s", path);
		 return;
	}
	lbuf = NULL;
	while ((buf = fgetln(f, &len))) {
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		else {
			/* EOF without EOL, copy and add the NUL */
			if ((lbuf = malloc(len + 1)) == NULL)
				err(1, NULL);
			memcpy(lbuf, buf, len);
			lbuf[len] = '\0';
			buf = lbuf;
		}
		for (a = buf; (b = strstr(a, MARKER_TAG)) != NULL; a = b+2) {
			*b = '\0';
			hputs(a);
			a = b+2;
			if ((b = strstr(a, MARKER_TAG)) != NULL) {
				*b = '\0';
				if (hput_generic_markers(a));
				else if (cb != NULL)
					cb(a, data);
			}
		}
		hputs(a);
		hputc('\n');
	}
	free(lbuf);
}
