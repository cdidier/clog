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

typedef void (parse_markers_cb)(const char *, void *);

struct data_ac {
	const char	*aname;
	const char	*title;
	const struct tm	*tm;
	FILE		*fbody;
	uint		*nb_comments;
};

struct data_p {
	page_cb		*cb;
	const char	*name;
};

int	read_article(const char *, article_cb, char *, size_t);
void	read_articles(article_cb);
ulong	read_article_tags(const char *, article_tag_cb);
void	read_tags(tag_cb);

#ifdef ENABLE_GZIP
#include <zlib.h>
extern gzFile	 gz;
#endif /* ENABLE_GZIP */

#ifdef ENABLE_COMMENTS
ulong	read_comments(const char *, comment_cb);

struct data_c {
	const char	*author;
	const struct tm	*tm;
	const char	*ip;
	const char	*mail;
	const char	*web;
	FILE		*fbody;
};

#ifdef ENABLE_POST_COMMENT
struct data_cf {
	const char	*aname;
	char		*hash;
	int		*jam1;
	int		*jam2;
};
#endif /* ENABLE_POST_COMMENT */

#endif /* ENABLE_COMMENTS */

#ifdef ENABLE_STATIC
time_t	get_mtime(const char *);
void	add_static_tag(const char *, long);
void	add_static_article(const char *);
extern int from_cmd, generating_static;
#endif /* ENABLE_STATIC */

#define MARKER_TAG "%%"

static uint	 nb_articles;
extern FILE	*hout;

static FILE *
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

static void
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

static void
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

static void
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

static void
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

static void
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

static void
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

static void
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

static void
markers_error(const char *m, void *data)
{
	const char *msg = data;

	if (strcmp(m, "MESSAGE") == 0)
		hputs(msg);
}

void
render_error(const char *msg)
{
	FILE *fin;

	if ((fin = open_template("error.html")) == NULL)
		return;
	parse_markers(fin, markers_error, (void *)msg);
	fclose(fin);
}

#ifdef ENABLE_COMMENTS

static void
markers_comment(const char *m, void *data)
{
	struct data_c *d = data;
	char date[TIME_LENGTH+1], body[BUFSIZ];

	if (strcmp(m, "NAME") == 0) {
		hput_escaped(d->author);
	} else if (strcmp(m, "DATE") == 0) {
		strftime(date, TIME_LENGTH+1, TIME_FORMAT, d->tm);
		hputs(date);
	} else if (strcmp(m, "IP") == 0) {
		hputs(d->ip);
	} else if (strcmp(m, "MAIL") == 0) {
		if (*d->mail != '\0') {
			hputs("<a href=\"mailto:");
			hput_escaped(d->mail);
			hputs("\">mail</a>");
		}
	} else if (strcmp(m, "WEB") == 0) {
		if (*d->web != '\0') {
			hputs("<a href=\"");
			hput_escaped(d->web);
			hputs("\">web</a>");
		}
	} else if (strcmp(m, "TEXT") == 0) {
		while (fgets(body, BUFSIZ, d->fbody) != NULL)
			hput_escaped(body);
	}
}

static void
render_comment(const char *author, const struct tm *tm, const char *ip,
    const char *mail, const char *web, FILE *fbody)
{
	FILE *fin;
	struct data_c d = { author, tm, ip, mail, web, fbody };

	if ((fin = open_template("comment.html")) == NULL)
		return;
	parse_markers(fin, markers_comment, &d);
	fclose(fin);
}

#ifdef ENABLE_POST_COMMENT

#define FORM_PREFIX	"FORM_"
#define FORM_LEN	sizeof("FORM_")-1

static void
markers_comment_form(const char *m, void *data)
{
	struct data_cf *d = data;
	extern struct cform comment_form;

	if (strcmp(m, "POST_URL") == 0) {
		hputs(BIN_URL"/");
		hputs(d->aname);
	} else if (strcmp(m, "JAM_HASH") == 0) {
		hputs(d->hash);
	} else if (strcmp(m, "JAM1") == 0) {
		hputs("&#");
		hputd(*d->jam1+48);
		hputc(';');
	} else if (strcmp(m, "JAM2") == 0) {
		hputs("&#");
		hputd(*d->jam2+48);
		hputc(';');
	} else if (strncmp(m, FORM_PREFIX, FORM_LEN) == 0) {
		if (strcmp(m+FORM_LEN, "NAME") == 0) {
			if (comment_form.name != NULL)
				hput_escaped(comment_form.name);
		} else if (strcmp(m+FORM_LEN, "MAIL") == 0) {
			if (comment_form.mail != NULL)
				hput_escaped(comment_form.mail);
		} else if (strcmp(m+FORM_LEN, "WEB") == 0) {
			if (comment_form.web != NULL)
				hput_escaped(comment_form.web);
		} else if (strcmp(m+FORM_LEN, "TEXT") == 0) {
			if (comment_form.text != NULL)
				hput_escaped(comment_form.text);
		} else if (strcmp(m+FORM_LEN, "ERROR") == 0) {
			if (comment_form.error != NULL)
				hputs(comment_form.error);
		}
	}
}

static void
render_comment_form(const char *aname)
{
	FILE *fin;
	int jam1, jam2;
	char salt[sizeof(JAM_SALT)+1], hash[SHA1_DIGEST_STRING_LENGTH];
	struct data_cf d = { aname, hash, &jam1, &jam2 };

	if ((fin = open_template("comment_form.html")) == NULL)
		return;
	srand(time(NULL));
	jam1 = rand()%(JAM_MAX-JAM_MIN+1) + JAM_MIN;
	jam2 = rand()%(JAM_MAX-JAM_MIN+1) + JAM_MIN;
	strlcpy(salt+1, JAM_SALT, sizeof(JAM_SALT));
	*salt = jam1+jam2;
	SHA1Data(salt, sizeof(JAM_SALT), hash);
	parse_markers(fin, markers_comment_form, &d);
	fclose(fin);
}

#endif /* ENABLE_POST_COMMENT */

static void
markers_comments(const char *m, void *data)
{
	const char *aname = data;

	if (strcmp(m, "COMMENTS") == 0)
		read_comments(aname, render_comment);
#ifdef ENABLE_POST_COMMENT
	else if (strcmp(m, "COMMENT_FORM") == 0)
		render_comment_form(aname);
#endif /* ENABLE_POST_COMMENT */
}

static void
render_comments(const char *aname)
{
	FILE *fin;

	if ((fin = open_template("comments.html")) == NULL)
		return;
	parse_markers(fin, markers_comments, (void *)aname);
	fclose(fin);
}

#endif /* ENABLE_COMMENTS */

static void
render_article_tag(const char *tname)
{
	hput_pagelink(tname, 0, tname);
	hputc(' ');
}

static void
markers_article_content(const char *m, void *data)
{
	struct data_ac *d = data;
	char date[TIME_LENGTH+1], body[BUFSIZ];

	if (strcmp(m, "TITLE") == 0) {
		hputs(d->title);
	} else if (strcmp(m, "DATE") == 0) {
		strftime(date, TIME_LENGTH+1, TIME_FORMAT, d->tm);
		hputs(date);
	} else if (strcmp(m, "TAGS") == 0) {
		if (read_article_tags(d->aname, render_article_tag) == 0)
			hputs(NO_TAG);
	} else if (strcmp(m, "BODY") == 0) {
		while (fgets(body, BUFSIZ, d->fbody) != NULL)
			hputs(body);
	} else if (strcmp(m, "URL") == 0) {
		hput_url(d->aname, NULL);
#ifdef ENABLE_COMMENTS
	} else if (strcmp(m, "NB_COMMENTS") == 0) {
		switch (*d->nb_comments) {
		case 0:
			hputs("no comment");
			break;
		case 1:
			hputs("1 comment");
			break;
		default:
			hputd(*d->nb_comments);
			hputs(" comments");
		}
	}
#endif /* ENABLE_COMMENTS */
}

static void
render_article_content(const char *aname, const char *title,
    const struct tm *tm, FILE *fbody, uint nb_comments)
{
	FILE *fin;
	struct data_ac d = { aname, title, tm, fbody, &nb_comments };

	if ((fin = open_template("article.html")) == NULL)
		return;
	parse_markers(fin, markers_article_content, &d);
	fclose(fin);
	++nb_articles;
}

void
render_article(const char *aname)
{
	if (aname == NULL)
		read_articles(render_article_content);
	else {
		if (read_article(aname, render_article_content, NULL, 0) == -1)
			render_error(ERR_PAGE_ARTICLE);
#ifdef ENABLE_COMMENTS
		else
			render_comments(aname);
#endif /* ENABLE_COMMENTS */
	}
}

static void
render_tag(const char *tag, uint nb_articles)
{
	hputs("<span style=\"font-size: ");
	hputd(nb_articles*TAG_CLOUD_THRES+100);
	hputs("%\">");
	hput_pagelink(tag, 0, tag);
	hputs("</span> ");
}

static void
markers_tags(const char *m, void *data)
{
	if (strcmp(m, "TAGS") == 0)
		read_tags(render_tag);
}

void
render_tags(const char *_)
{
	FILE *fin;

	if ((fin = open_template("tags.html")) == NULL)
		return;
	parse_markers(fin, markers_tags, NULL);
	fclose(fin);
}

static void
markers_page(const char *m, void *data)
{
	struct data_p *d = data;
	extern const char *tag;
	extern long offset;

	if (strcmp(m, "TITLE") == 0) {
		if (d->cb == render_article && d->name == NULL) {
			if (tag != NULL) {
				hputs("- tag:");
				hputs(tag);
			}
		} else if (d->cb == render_article) {
			char title[BUFSIZ];

			*title = '\0';
			read_article(d->name, NULL, title, BUFSIZ);
			if (*title != '\0') {
				hputs("- ");
				hputs(title);
			}
		} else if (d->cb == render_tags)
			hputs("- tags");
	} else if (strcmp(m, "NEXT") == 0) {
		if (d->cb == render_article && offset > 0)
			hput_pagelink(tag, offset-1, NAV_NEXT);
	} else if (strcmp(m, "PREVIOUS") == 0) {
		if (d->cb == render_article && nb_articles > NB_ARTICLES)
			hput_pagelink(tag, offset+1, NAV_PREVIOUS);
	} else if (strcmp(m, "BODY") == 0) {
		if (d->cb != NULL)
			d->cb(d->name);
	}
}

void
render_page(page_cb cb, const char *name)
{
	FILE *fin;
	struct data_p d = { cb, name };

	nb_articles = 0;
	if ((fin = open_template("page.html")) == NULL)
		return;
#ifdef ENABLE_STATIC
	if (!generating_static) {
#endif /* ENABLE_STATIC */
#ifdef ENABLE_GZIP
		if (gz != NULL)
			fputs("Content-Encoding: gzip\r\n", stdout);
#endif /* ENABLE_GZIP */
		fputs("Content-type: text/html;"
		    "charset="CHARSET"\r\n\r\n", hout);
		fflush(hout);
#ifdef ENABLE_STATIC
	}
	if (from_cmd && cb == render_article && name != NULL) {
		time_t mtime;

		if ((mtime = get_mtime(name)) != 0) {
			char buf[BUFSIZ];

			strftime(buf, BUFSIZ, MOD_FORMAT, localtime(&mtime));
			hputs(MOD_BEGIN);
			hputs(buf);
			hputs(MOD_END);
			hputc('\n');
		}
	}
#endif /* ENABLE_STATIC */
	parse_markers(fin, markers_page, &d);
	fclose(fin);
}

#define RSS_DATE_FORMAT "%a, %d %b %Y %R:00 %z"
#define RSS_DATE_LENGTH 32

static void
render_rss_article_tag(const char *tname)
{
	if (tname == NULL)
		return;
	hputs(
	    "      <category>");
	hputs(tname);
	hputs("</category>\n");
}

static void
render_rss_article(const char *aname, const char *title, const struct tm *tm,
    FILE *fbody, uint _)
{
	char body[BUFSIZ], date[RSS_DATE_LENGTH];

	hputs(
	    "    <item>\n"
	    "      <title>");
	hputs(title);
	hputs("</title>\n"
	    "      <link>");
	hput_url(aname, NULL);
	hputs("</link>\n");
	read_article_tags(aname, render_rss_article_tag);
	hputs(
	    "      <description><![CDATA[");
	while (fgets(body, BUFSIZ, fbody) != NULL)
		hputs(body);
	hputs("]]></description>\n"
	    "      <pubDate>");
	strftime(date, RSS_DATE_LENGTH, RSS_DATE_FORMAT, tm);
	hputs(date);
	hputs("</pubDate>\n"
	    "      <guid isPermaLink=\"false\">");
	hputs(aname);
	hputs("</guid>\n"
	    "    </item>\n");
}

void
render_rss(void)
{
	char date[RSS_DATE_LENGTH];
	time_t now;
	extern const char *tag;

#ifdef ENABLE_STATIC
	if (!generating_static) {
#ifdef ENABLE_GZIP
		if (gz != NULL)
			fputs("Content-Encoding: gzip\r\n", stdout);
#endif /* ENABLE_GZIP */
		fputs("Content-type: application/rss+xml;"
		    "charset="CHARSET"\r\n\r\n", hout);
		fflush(hout);
	}
#else
	if (gz != NULL)
		fputs("Content-Encoding: gzip\r\n", stdout);
	fputs("Content-type: application/rss+xml;charset="CHARSET"\r\n\r\n",
	    hout);
	fflush(hout);
#endif /* ENABLE_STATIC */
	hputs(
	    "<?xml version=\"1.0\"?>\n"
	    "<rss version=\"2.0\" xmlns:atom=\"http://www.w3.org/2005/Atom\">\n"
	    "  <channel>\n"
	    "    <atom:link href=\"");
	hput_url("rss", tag);
	hputs("\" rel=\"self\" type=\"application/rss+xml\" />\n"
	    "    <title>");
	hputs(SITE_NAME);
	if (tag != NULL) {
		hputs(" - tag:");
		hputs(tag);
	}
	hputs("</title>\n"
	    "    <link>");
	hput_url(NULL, NULL);
	hputs("</link>\n"
	    "    <description>");
	hputs(DESCRIPTION);
	hputs("</description>\n"
	    "    <pubDate>");
	time(&now);
	strftime(date, RSS_DATE_LENGTH, RSS_DATE_FORMAT, localtime(&now));
	hputs(date);
	hputs("</pubDate>\n");
	read_articles(render_rss_article);
	hputs(
	    "  </channel>\n"
	    "</rss>\n");
}
