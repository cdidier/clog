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

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <zlib.h>
#include "common.h"
#include "output.h"
#include "articles.h"
#include "comments.h"

static void
markers_comment(const char *m, struct comment *c)
{
	char date[BUFSIZ], *buf, *a, *b, ch;
	int freeln;

	if (strcmp(m, "COMMENT_AUTHOR") == 0) {
		if (!EMPTYSTRING(c->mail)) {
			hputs("<a href=\"mailto:");
			hput_escaped(c->mail);
			hputs("\">");
		}
		hput_escaped(c->author);
		if (!EMPTYSTRING(c->mail))
			hputs("</a>");
	} else if (strcmp(m, "COMMENT_NB") == 0) {
		hputd(c->number);
	} else if (strcmp(m, "COMMENT_DATE") == 0) {
		strftime(date, sizeof(date), TIME_FORMAT, localtime(&c->date));
		hputs(date);
	} else if (strcmp(m, "COMMENT_IP") == 0) {
		if (!EMPTYSTRING(c->ip))
			hputs(c->ip);
	} else if (strcmp(m, "COMMENT_MAIL") == 0) {
		if (!EMPTYSTRING(c->mail)) {
			hputs("<a href=\"mailto:");
			hput_escaped(c->mail);
			hputs("\">mail</a>");
		}
	} else if (strcmp(m, "COMMENT_WEB") == 0) {
		if (!EMPTYSTRING(c->web)) {
			hputs("<a href=\"");
			hput_escaped(c->web);
			hputs("\">web</a>");
		}
	} else if (strcmp(m, "COMMENT_TEXT") == 0) {
		while ((buf = read_commentln(c, &freeln))) {
			/* disply url as link */
			for (a = buf; (b = strstr(a, "http://")) != NULL
			    || (b = strstr(a, "https://")) != NULL; a = b) {
				*b = '\0';
				hput_escaped(a);
				*b = 'h';
				a = b;
				for (ch = '\0'; ch == '\0' && *b != '\0' ; ++b)
					if (isspace(*b)
					    || (*b == '.' && (isspace(b[1]) ||
					    b[1] == '\0'))
					    || (*b == ')' && (isspace(b[1]) ||
					    b[1] == '.' || b[1] == '\0'))) {
						ch = *b;
						*b = '\0';
					}
				--b;
				hputs("<a href=\"");
				hput_escaped(a);
				hputs("\">");
				hput_escaped(a);
				hputs("</a>");
				*b = ch;
			}
			hput_escaped(a);
			hputs("<br>\n");
			if (freeln)
				free(buf);
		}
	}
}

static void
render_comment(struct comment *c)
{
	parse_template("article_comment.html", (markers_cb *)markers_comment,
	    c);
}

static void
markers_comment_form(const char *m, struct article *a)
{
	extern char *error_str;
	extern struct query *query_post;
	char *s;

	if (strcmp(m, "FORM_POST_URL") == 0) {
		hputs(BIN_URL "?article=");
		hputs(a->name);
	} else if (strcmp(m, "ANTISPAM_JAM1") == 0) {
		if (a->antispam != NULL) {
			hputs("&#");
			hputd(a->antispam->jam1+48);
			hputc(';');
		}
	} else if (strcmp(m, "ANTISPAM_JAM2") == 0) {
		if (a->antispam != NULL) {
			hputs("&#");
			hputd(a->antispam->jam2+48);
			hputc(';');
		}
	} else if (strcmp(m, "ANTISPAM_HASH") == 0) {
		if (a->antispam != NULL)
			hputs(a->antispam->hash);
	} else if (error_str == NULL)
		return; /* ignore the following if there is no error */
	if (strcmp(m, "FORM_ERROR") == 0) {
		hputs("<span style=\"color: red;\"><b>");
		hputs(error_str);
		hputs("</b></span>");
	} else if (strcmp(m, "FORM_AUTHOR") == 0) {
		if ((s = get_query_param(query_post, "author")) != NULL)
			hput_escaped(s);
	} else if (strcmp(m, "FORM_MAIL") == 0) {
		if ((s = get_query_param(query_post, "mail")) != NULL)
			hput_escaped(s);
	} else if (strcmp(m, "FORM_WEB") == 0) {
		if ((s = get_query_param(query_post, "web")) != NULL)
			hput_escaped(s);
	} else if (strcmp(m, "FORM_TEXT") == 0) {
		if ((s = get_query_param(query_post, "text")) != NULL)
			hput_escaped(s);
	}
}

static void
markers_article(const char *m, struct article *a)
{
	struct article_tag *at;
	char buf[BUFSIZ];
	ulong nb_comments;
	extern char *error_str;

	if (strcmp(m, "ARTICLE_TITLE") == 0) {
		hputs(a->title);
	} else if (strcmp(m, "ARTICLE_DATE") == 0) {
		strftime(buf, sizeof(buf), TIME_FORMAT, &a->date);
		hputs(buf);
	} else if (strcmp(m, "ARTICLE_TAGS") == 0) {
		SLIST_FOREACH(at, &a->tags, next) {
			if (at->name == NULL)
				continue;
			hputs("<a href=\"");
			hput_url("tag", at->name, 0, NB_ARTICLES);
			hputs("\">");
			hputs(at->name);
			hputs("</a>");
			if (SLIST_NEXT(at, next) != NULL)
				hputs(" / ");
		}
	} else if (strcmp(m, "ARTICLE_BODY") == 0) {
		while (fgets(buf, sizeof(buf), a->body) != NULL)
			hputs(buf);
		if (a->more != NULL) {
			if (a->display_more)
				while (fgets(buf, sizeof(buf), a->more) != NULL)
					hputs(buf);
			else {
				hputs("<b><a href=\"");
				hput_url("article", a->name);
				hputs("\">" NAVIGATION_READMORE "</a></b>");
				if (a->more_size != 0) {
					hputs(" (");
					hputd(a->more_size);
					hputc(' ');
					hputs(NAVIGATION_BYTES);
					hputc(')');
				}
			}
		}
	} else if (strcmp(m, "ARTICLE_URL") == 0) {
		hput_url("article", a->name);
	} else if (strcmp(m, "ARTICLE_COMMENTS_INFO") == 0) {
		if (are_comments_readable(a->name)
		    || are_comments_writable(a->name)) {
			nb_comments = read_comments(a->name, NULL);
			hputs("<a href=\"");
			hput_url("article", a->name);
			hputs("#coms\">[");
			switch (nb_comments) {
			case 0:
				hputs(COMMENTS_NOCOMMENT);
				break;
			case 1:
				hputs(COMMENTS_1COMMENT);
				break;
			default:
				hputd(nb_comments);
				hputc(' ');
				hputs(COMMENTS_COMMENTS);
			}
			hputs("]</a>");
		}
	} else if (strcmp(m, "ARTICLE_COMMENTS") == 0) {
		if (a->display_more) {
			read_comments(a->name, render_comment);
			if (are_comments_writable(a->name)
			    || error_str != NULL) {
				a->antispam = antispam_generate(a->name);
				parse_template("article_comment_form.html",
				    (markers_cb *)markers_comment_form, a);
				free(a->antispam);
				a->antispam = NULL;
			}
		}
	}
}

static void
markers_page_article(const char *m, struct article *a) 
{
	if (strcmp(m, "PAGE_TITLE") == 0) {
		hputs(" - ");
		hputs(a->title);
	} else if (strcmp(m, "PAGE_BODY") == 0) {
		a->display_more = 1;
		parse_template("article.html", (markers_cb *)markers_article,
		    a);
	}
}

void
render_page_article(struct article *a)
{
	open_output("text/html");
	parse_template("page.html", (markers_cb *)markers_page_article, a);
	close_output();
}

static void
render_tag_article(struct article *a)
{
	a->display_more = 0;
	parse_template("article.html", (markers_cb *)markers_article, a);
}

static void
markers_page_tag(const char *m, struct tag *t)
{
	if (strcmp(m, "PAGE_TITLE") == 0) {
		if (t->name != NULL) {
			hputs(" - tag:");
			hputs(t->name);
		}
	} else if (strcmp(m, "HEADERS") == 0) {
		/* link to the RSS feed of the tag */
		hputs("\t<link rel=\"alternate\" type=\"application/rss+xml\" "
		    "title=\"" SITE_NAME " - RSS");
		if (t->name != NULL) {
			hputs(" - tag:");
			hputs(t->name);
		}
		hputs("\" href=\"");
		hput_url("rss", t->name);
		hputs("\">\n");
		/* no cache */
		hputs("\t<meta http-equiv=\"cache-control\" content=\"no-cache\">");
	} else if (strcmp(m, "NAVIGATION_NEXT") == 0) {
		if (t->next) {
			hputs("<a href=\"");
			hput_url("tag", t->name, t->page+1, t->number);
			hputs("\">" NAVIGATION_NEXT "</a>");
		}
	} else if (strcmp(m, "NAVIGATION_PREVIOUS") == 0) {
		if (t->previous) {
			hputs("<a href=\"");
			hput_url("tag", t->name, t->page-1, t->number);
			hputs("\">" NAVIGATION_PREVIOUS "</a>");
		}
	} else if (strcmp(m, "NAVIGATION_PAGE") == 0) {
		hputd(t->page+1);
	} else if (strcmp(m, "NAVIGATION_PAGES") == 0) {
		hputd(t->pages);
	} else if (strcmp(m, "PAGE_BODY") == 0) {
		read_articles(t->name, t->offset, t->number,
		    render_tag_article);
	}
}

void
render_page_tag(struct tag *t)
{
	open_output("text/html");
	parse_template("page.html", (markers_cb *)markers_page_tag, t);
	close_output();
}

static void
markers_page_tags2(const char *m)
{
	SLIST_HEAD(, article_tag) list;
	struct article_tag *at;
	unsigned long nb_articles_total, nb_articles;


	if (strcmp(m, "TAGS") != 0)
		return;
	nb_articles_total = read_articles(NULL, 0, 0, NULL);
	SLIST_FIRST(&list) = get_article_tags(NULL);
	SLIST_FOREACH(at, &list, next) {
		if (at->name == NULL)
			return;
		nb_articles = read_articles(at->name, 0, 0, NULL);
		if (nb_articles == 0)
			continue;
		hputs("<span style=\"font-size: ");
		hputd(nb_articles * (100/nb_articles_total)+100);
		hputs("%\"><a href=\"");
		hput_url("tag", at->name, 0, NB_ARTICLES);
		hputs("\">");
		hputs(at->name);
		hputs("</a></span> ");
	}
	while (!SLIST_EMPTY(&list)) {
		at = SLIST_FIRST(&list);
		SLIST_REMOVE_HEAD(&list, next);
		free(at->name);
		free(at);
	}
}

static void
markers_page_tags(const char *m)
{
	if (strcmp(m, "PAGE_TITLE") == 0)
		hputs(" - tags");
	else if (strcmp(m, "PAGE_BODY") == 0)
		parse_template("tags.html", (markers_cb *)markers_page_tags2,
		    NULL);
}

void
render_page_tags(void)
{
	open_output("text/html");
	parse_template("page.html", (markers_cb *)markers_page_tags, NULL);
	close_output();
}

static void
render_rss_article(struct article *a)
{
	char buf[BUFSIZ];
	struct article_tag *at;

	hputs("    <item>\n"
	    "      <title>");
	hputs(a->title);
	hputs("</title>\n"
	    "      <link>");
	hput_url("article", a->name);
	hputs("</link>\n");
	SLIST_FOREACH(at, &a->tags, next) {
		if (at->name == NULL)
			continue;
		hputs("      <category>");
		hputs(at->name);
		hputs("</category>\n");
	}
	hputs("      <description><![CDATA[");
	while (fgets(buf, sizeof(buf), a->body) != NULL)
		hputs(buf);
	if (a->more != NULL) {
		hputs("<b><a href=\"");
		hput_url("article", a->name);
		hputs("\">" NAVIGATION_READMORE "</a></b>");
	}
	hputs("]]></description>\n"
	    "      <pubDate>");
	strftime(buf, sizeof(buf), "%a, %d %b %Y %R %z", &a->date);
	hputs(buf);
	hputs("</pubDate>\n"
	    "      <guid isPermaLink=\"false\">");
	hputs(a->name);
	hputs("</guid>\n"
	    "    </item>\n");
}

void
render_rss(struct tag *t)
{
	char date[32];
	time_t now;
	extern enum STATUS status;

	open_output("application/rss+xml");
	hputs(
	    "<?xml version=\"1.0\"?>\n"
	    "<rss version=\"2.0\" xmlns:atom=\"http://www.w3.org/2005/Atom\">\n"
	    "  <channel>\n"
	    "    <atom:link href=\"");
	if (status & STATUS_STATIC)
		hput_url("rss", t->name);
	else
		hput_url("rss", NULL); /* XXX problem with '&' */
	hputs("\" rel=\"self\" type=\"application/rss+xml\" />\n"
	    "    <title>");
	hputs(SITE_NAME);
	if (t->name != NULL) {
		hputs(" - tag:");
		hputs(t->name);
	}
	hputs("</title>\n"
	    "    <link>");
	hputs(BASE_URL);
	hputs("</link>\n"
	    "    <description>");
	hputs(DESCRIPTION);
	hputs("</description>\n"
	    "    <pubDate>");
	time(&now);
	strftime(date, sizeof(date), "%a, %d %b %Y %R %z", localtime(&now));
	hputs(date);
	hputs("</pubDate>\n");
	read_articles(t->name, t->offset, t->number, render_rss_article);
	hputs(
	    "  </channel>\n"
	    "</rss>\n");
	close_output();
}
