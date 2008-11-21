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
#include <zlib.h>
#include <sys/types.h>
#include <sha1.h>

#include "common.h"

int	read_article(char *, article_cb, char *, size_t);
void	read_articles(article_cb);
uint	read_article_tags(char *, article_tag_cb);
void	read_tags(tag_cb);
#if defined(ENABLE_COMMENTS) && ENABLE_COMMENTS == 1
uint	read_comments(char *, comment_cb);
#endif /* ENABLE_COMMENTS */

#define TAG "%%"

static uint	nb_articles;
extern gzFile	gz;

static void
hputc(const char c)
{
	if (gz != NULL)
		gzputc(gz, c);
	else
		fputc(c, stdout);
}

static void
hputs(const char *s)
{
	if (gz != NULL)
		gzputs(gz, s);
	else
		fputs(s, stdout);
}

static void
hputd(const long long l)
{
	if (gz != NULL)
		gzprintf(gz, "%lld", l);
	else
		fprintf(stdout, "%lld", l);
}

static void
hput_escaped(char *s)
{
	char *a, *b;
	char c;

	for (a = s; (b = strpbrk(a, "<>'\"&\n")) != NULL; a = b+1) {
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
			hputs("<br>");
			break;
		default:
			hputc(c);
		}
	}
	hputs(a);
}

static void
hput_link(char *first_level, char *second_level, char *text)
{
	hputs("<a href=\""BASE_URL);
	if (first_level != NULL) {
		if (*first_level != '?')
			hputc('/');
		hputs(first_level);
		if (second_level != NULL) {
			if (*second_level != '?')
				hputc('/');
			hputs(second_level);
		}
	}
	hputs("\">");
	hputs(text);
	hputs("</a>");
}

static void
hput_pagelink(long page, char *text)
{
	hputs("<a href=\""BASE_URL);
	if (page > 0) {
		hputs("?p=");
		hputd(page);
	}
	hputs("\">");
	hputs(text);
	hputs("</a>");
}

void
render_error(char *msg)
{
	FILE *fin;
	char buf[BUFSIZ], *a, *b;

	if ((fin = fopen(TEMPLATES_DIR"/error.html", "r")) == NULL) {
		warn("fopen: %s", TEMPLATES_DIR"/error.html");
		return;
	}
	while (fgets(buf, BUFSIZ, fin) != NULL) {
		buf[strcspn(buf, "\n")] = '\0';
		for (a = buf; (b = strstr(a, TAG)) != NULL; a = b+2) {
			*b = '\0';
			hputs(a);
			a = b+2;
			if ((b = strstr(a, TAG)) != NULL) {
				*b = '\0';
				if (strcmp(a, "MESSAGE") == 0)
					hputs(msg);
			}
		}
		hputs(a);
		hputc('\n');
	}
	fclose(fin);
}

static void
render_article_tag(char *tag)
{
	if (tag == NULL)
		hputs("none");
	else {
		hput_link("tag", tag, tag);
		hputc(' ');
	}
}

static int
render_generic_markers(char *a)
{ 
	extern char *tag;

	if (strcmp(a, "BASE_URL") == 0)
		hputs(BASE_URL);
	else if (strcmp(a, "SITE_NAME") == 0)
		hputs(SITE_NAME);
	else if (strcmp(a, "DESCRIPTION") == 0)
		hputs(DESCRIPTION);
	else if (strcmp(a, "COPYRIGHT") == 0)
		hputs(COPYRIGHT);
	else if (strcmp(a, "CHARSET") == 0)
		hputs(CHARSET);
	else if (strcmp(a, "RSS") == 0) {
		hputs(BASE_URL"/rss");
		if (tag != NULL) {
			hputc('/');
			hputs(tag);
		}
	} else
		return 0;
	return 1;
}

#if defined(ENABLE_COMMENTS) && ENABLE_COMMENTS == 1

static void
render_comment(char *author, struct tm *tm, char *ip, char *mail, char *web,
    FILE *fbody)
{
	FILE *fin;
	char buf[BUFSIZ], *a, *b;
	char date[TIME_LENGTH+1], body[BUFSIZ];

	if ((fin = fopen(TEMPLATES_DIR"/comment.html", "r")) == NULL) {
		warn("fopen: %s", TEMPLATES_DIR"/comment.html");
		return;
	}
	while (fgets(buf, BUFSIZ, fin) != NULL) {
		buf[strcspn(buf, "\n")] = '\0';
		for (a = buf; (b = strstr(a, TAG)) != NULL; a = b+2) {
			*b = '\0';
			hputs(a);
			a = b+2;
			if ((b = strstr(a, TAG)) != NULL) {
				*b = '\0';
				if (render_generic_markers(a));
				else if (strcmp(a, "NAME") == 0)
					hput_escaped(author);
				else if (strcmp(a, "DATE") == 0) {
					strftime(date, TIME_LENGTH+1,
					    TIME_FORMAT, tm);
					hputs(date);
				} else if (strcmp(a, "IP") == 0)
					hputs(ip);
				else if (*mail != '\0'
				    && strcmp(a, "MAIL") == 0) {
					hputs("<a href=\"mailto:");
					hput_escaped(mail);
					hputs("\">mail</a>");
				} else if (*web != '\0'
				    && strcmp(a, "WEB") == 0) {
					hputs("<a href=\"");
					hput_escaped(web);
					hputs("\">web</a>");
				} else if (strcmp(a, "TEXT") == 0) {
					while (fgets(body, BUFSIZ,
					    fbody) != NULL)
						hput_escaped(body);
				}
			}
		}
		hputs(a);
		hputc('\n');
	}
	fclose(fin);
}

#if defined(ENABLE_POST_COMMENT) && ENABLE_POST_COMMENT == 1

static void
render_comment_form(void)
{
	FILE *fin;
	char buf[BUFSIZ], *a, *b;
	int jam1, jam2;
	char salt[sizeof(JAM_SALT)+1], hash[SHA1_DIGEST_STRING_LENGTH];
	extern struct cform comment_form;

	if ((fin = fopen(TEMPLATES_DIR"/comment_form.html", "r")) == NULL) {
		warn("fopen: %s", TEMPLATES_DIR"/comment_form.html");
		return;
	}
	srand(time(NULL));
	jam1 = rand()%(JAM_MAX-JAM_MIN+1) + JAM_MIN;
	jam2 = rand()%(JAM_MAX-JAM_MIN+1) + JAM_MIN;
	strlcpy(salt+1, JAM_SALT, sizeof(JAM_SALT));
	*salt = jam1+jam2;
	SHA1Data(salt, sizeof(JAM_SALT), hash);
	while (fgets(buf, BUFSIZ, fin) != NULL) {
		buf[strcspn(buf, "\n")] = '\0';
		for (a = buf; (b = strstr(a, TAG)) != NULL; a = b+2) {
			*b = '\0';
			hputs(a);
			a = b+2;
			if ((b = strstr(a, TAG)) != NULL) {
				*b = '\0';
				if (render_generic_markers(a));
				else if (strcmp(a, "JAM_HASH") == 0)
					hputs(hash);
				else if (strcmp(a, "JAM1") == 0) {
					hputs("&#");
					hputd(jam1+48);
					hputc(';');
				} else if (strcmp(a, "JAM2") == 0) {
					hputs("&#");
					hputd(jam2+48);
					hputc(';');
				} else if (comment_form.name != NULL
				    && strcmp(a, "FORM_NAME") == 0)
					hput_escaped(comment_form.name);
				else if (comment_form.mail
				    && strcmp(a, "FORM_MAIL") == 0)
					hput_escaped(comment_form.mail);
				else if (comment_form.web != NULL
				    && strcmp(a, "FORM_WEB") == 0)
					hput_escaped(comment_form.web);
				else if (comment_form.text != NULL
				    && strcmp(a, "FORM_TEXT") == 0)
					hput_escaped(comment_form.text);
				else if (comment_form.error != NULL
				    && strcmp(a, "FORM_ERROR") == 0)
					hputs(comment_form.error);
			}
		}
		hputs(a);
		hputc('\n');
	}
	fclose(fin);
}

#endif /* ENABLE_POST_COMMENT */

static void
render_comments(char *name)
{
	FILE *fin;
	char buf[BUFSIZ], *a, *b;

	if ((fin = fopen(TEMPLATES_DIR"/comments.html", "r")) == NULL) {
		warn("fopen: %s", TEMPLATES_DIR"/comments.html");
		return;
	}
	while (fgets(buf, BUFSIZ, fin) != NULL) {
		buf[strcspn(buf, "\n")] = '\0';
		for (a = buf; (b = strstr(a, TAG)) != NULL; a = b+2) {
			*b = '\0';
			hputs(a);
			a = b+2;
			if ((b = strstr(a, TAG)) != NULL) {
				*b = '\0';
				if (render_generic_markers(a));
				else if (strcmp(a, "COMMENTS") == 0)
					read_comments(name,
					    render_comment);
#if defined(ENABLE_POST_COMMENT) && ENABLE_POST_COMMENT == 1
				else if (strcmp(a, "COMMENT_FORM") == 0)
					render_comment_form();
#endif /* ENABLE_POST_COMMENT */
			}
		}
		hputs(a);
		hputc('\n');
	}
	fclose(fin);
}

#endif /* ENABLE_COMMENTS */

static void
render_article_content(char *aname, char *title, struct tm *tm, FILE *fbody,
    uint nb_comments)
{
	FILE *fin;
	char buf[BUFSIZ], *a, *b;
	char date[TIME_LENGTH+1], body[BUFSIZ];

	if ((fin = fopen(TEMPLATES_DIR"/article.html", "r")) == NULL) {
		warn("fopen: %s", TEMPLATES_DIR"/article.html");
		return;
	}
	while (fgets(buf, BUFSIZ, fin) != NULL) {
		buf[strcspn(buf, "\n")] = '\0';
		for (a = buf; (b = strstr(a, TAG)) != NULL; a = b+2) {
			*b = '\0';
			hputs(a);
			a = b+2;
			if ((b = strstr(a, TAG)) != NULL) {
				*b = '\0';
				if (render_generic_markers(a));
				else if (strcmp(a, "TITLE") == 0) {
					hputs(title);	
				} else if (strcmp(a, "DATE") == 0) {
					strftime(date, TIME_LENGTH+1,
					    TIME_FORMAT, tm);
					hputs(date);
				} else if (strcmp(a, "TAGS") == 0) {
					read_article_tags(aname,
					    render_article_tag);
				} else if (strcmp(a, "BODY") == 0) {
					while (fgets(body, BUFSIZ,
					    fbody) != NULL)
						hputs(body);
				} else if (strcmp(a, "URL") == 0) {
					hputs(BASE_URL);
					hputc('/');
					hputs(aname);
				} else if (strcmp(a, "NB_COMMENTS") == 0) {
#if defined(ENABLE_COMMENTS) && ENABLE_COMMENTS == 1
					switch (nb_comments) {
					case 0:
						hputs("no comment");
						break;
					case 1:
						hputs("1 comment");
						break;
					default:
						hputd(nb_comments);
						hputs(" comments");
					}
#endif /* ENABLE_COMMENTS */
				}
			}
		}
		hputs(a);
		hputc('\n');
	}
	fclose(fin);
	++nb_articles;
}

void
render_article(char *aname)
{
	if (aname == NULL)
		read_articles(render_article_content);
	else {
		if (read_article(aname, render_article_content, NULL, 0) == -1)
			render_error(ERR_PAGE_ARTICLE);
#if defined(ENABLE_COMMENTS) && ENABLE_COMMENTS == 1
		else
			render_comments(aname);
#endif /* ENABLE_COMMENTS */
	}
}

static void
render_tag(char *tag, uint nb_articles)
{
	hputs("<span style=\"font-size: ");
	hputd(nb_articles*TAG_CLOUD_THRES+100);
	hputs("%\">");
	hput_link("tag", tag, tag);
	hputs("</span> ");
}

void
render_tags(char *nop)
{
	FILE *fin;
	char buf[BUFSIZ], *a, *b;

	if ((fin = fopen(TEMPLATES_DIR"/tags.html", "r")) == NULL) {
		warn("fopen: %s", TEMPLATES_DIR"/error.html");
		return;
	}
	while (fgets(buf, BUFSIZ, fin) != NULL) {
		buf[strcspn(buf, "\n")] = '\0';
		for (a = buf; (b = strstr(a, TAG)) != NULL; a = b+2) {
			*b = '\0';
			hputs(a);
			a = b+2;
			if ((b = strstr(a, TAG)) != NULL) {
				*b = '\0';
				if (render_generic_markers(a));
				else if (strcmp(a, "TAGS") == 0)
					read_tags(render_tag);
			}
		}
		hputs(a);
		hputc('\n');
	}
	fclose(fin);
}

void
render_page(page_cb cb, char *data)
{
	FILE *fin;
	char buf[BUFSIZ], *a, *b;
	extern char *tag;
	extern long offset;

	nb_articles = 0;
	if ((fin = fopen(TEMPLATES_DIR"/page.html", "r")) == NULL) {
		warn("fopen: %s", TEMPLATES_DIR"/page.html");
		return;
	}
	fputs("Content-type: text/html;charset="CHARSET"\r\n\r\n", stdout);
	fflush(stdout);
	while (fgets(buf, BUFSIZ, fin) != NULL) {
		buf[strcspn(buf, "\n")] = '\0';
		for (a = buf; (b = strstr(a, TAG)) != NULL; a = b+2) {
			*b = '\0';
			hputs(a);
			a = b+2;
			if ((b = strstr(a, TAG)) != NULL) {
				*b = '\0';
				if (render_generic_markers(a));
				else if (strcmp(a, "TITLE") == 0) {
					if (cb == render_article
					    && data == NULL) {
						if (tag != NULL) {
							hputs("- tag:");
							hputs(tag);
						}
					} else if (cb == render_article) {
						char title[BUFSIZ];

						*title = '\0';
						read_article(data, NULL,
						    title, BUFSIZ);
						if (*title != '\0') {
							hputs("- ");
							hputs(title);
						}
					} else if (cb == render_tags)
						hputs("- tags");
				} else if (strcmp(a, "NEXT") == 0
				    && cb == render_article
				    && offset > 0)
					hput_pagelink(offset-1, NAV_NEXT);
				else if (strcmp(a, "PREVIOUS") == 0
				    && nb_articles >= NB_ARTICLES) {
					hput_pagelink(offset+1, NAV_PREVIOUS);
				} else if (strcmp(a, "BODY") == 0
				    && cb != NULL)
					cb(data);
			}
		}
		hputs(a);
		hputc('\n');
	}
	fclose(fin);
}

#define RSS_DATE_FORMAT "%a, %d %b %Y %R:00 %z"
#define RSS_DATE_LENGTH 32

static void
render_rss_article(char *aname, char *title, struct tm *tm, FILE *fbody,
    uint nop)
{
	char body[BUFSIZ], date[RSS_DATE_LENGTH];

	hputs(
	    "    <item>\n"
	    "      <title>");
	hputs(title);
	hputs("</title>\n"
	    "      <link>");
	hputs(BASE_URL);
	hputc('/');
	hputs(aname);
	hputs("</link>\n"
	    "      <description>");
	while (fgets(body, BUFSIZ, fbody) != NULL)
		hputs(body);
	hputs(
	    "      </description>\n"
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
	extern char *tag;

	fputs("Content-type: application/rss+xml;charset="CHARSET"\r\n\r\n",
	    stdout);
	fflush(stdout);
	hputs(
	    "<?xml version=\"1.0\"?>\n"
	    "<rss version=\"2.0\" xmlns:atom=\"http://www.w3.org/2005/Atom\">\n"
	    "  <channel>\n"
	    "    <atom:link href=\"");
	hputs(BASE_URL"/rss");
	if (tag != NULL) {
		hputc('/');
		hputs(tag);
	}
	hputs("\" rel=\"self\" type=\"application/rss+xml\" />\n"
	    "    <title>");
	hputs(SITE_NAME);
	hputs("</title>\n"
	    "    <link>");
	hputs(BASE_URL);
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
