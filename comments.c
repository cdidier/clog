/*
 * $Id$
 *
 * Copyright (c) 2009 Colin Didier <cdidier@cybione.org>
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
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"
#include "antispam.h"
#include "comments.h"

void	strchomp(char *);
time_t	rfc822_date(char *);

#define FROM_	"From "
#define AUTHOR	"From: "
#define DATE	"Date: "
#define IP	"X-IP: "
#define WEB	"X-Website: "

#define MODE_READ	"r"
#define MODE_APPEND	"a"

#define INPUT_LEN	128
#define TEXT_LEN	2048
#define MAX_COMMENT_LEN	(4*INPUT_LEN+TEXT_LEN+128)

static FILE *
open_comments_file(const char *article, const char *mode)
{
	char path[MAXPATHLEN];
	int fd;
	FILE *f;
	extern char status;

	snprintf(path, MAXPATHLEN, "%s" ARTICLES_DIR "/%s/comments",
	    status & STATUS_FROMCMD ? CHROOT_DIR : "", article);
	if ((fd = open(path,
	    mode == MODE_APPEND ? O_WRONLY|O_APPEND : O_RDONLY , 0)) == -1)
		return NULL;
	if ((f = fdopen(fd, mode)) == NULL) {
		close(fd);
		return NULL;
	}
	return f;
}

int
are_comments_writable(const char *article)
{
	FILE *f;

	if ((f = open_comments_file(article, MODE_APPEND)) == NULL)
		return 0;
	fclose(f);
	return 1;
}

static int
write_comment(const char *article, const char *author, const char *mail,
    const char *web, char *text)
{
	FILE *f;
	int fd;
	struct flock fl;
	char host[MAXHOSTNAMELEN], date[32];
	char *p, *s, *buf, c;
	time_t now;
	size_t len;

	if ((f = open_comments_file(article, MODE_APPEND)) == NULL)
		goto err;
	/* lock */
	fd = fileno(f);
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_pid = getpid();
	if (fcntl(fd, F_SETLKW, &fl) == -1)
		goto err;
	if (gethostname(host, MAXHOSTNAMELEN) == -1)
		goto err2;
	time(&now);
	fprintf(f, FROM_ "%s@%s %s", user_from_uid(getuid(), 0), host,
	    ctime(&now));
	if ((len = strlen(author)) > INPUT_LEN)
		len = INPUT_LEN;
	fputs(AUTHOR, f);
	fwrite(author, 1, len, f);
	if (!EMPTYSTRING(mail))
		fprintf(f, " <%s>", mail);
	fputc('\n', f);
	strftime(date, sizeof(date), "%a, %d %b %Y %H:%M %z", localtime(&now));
	fprintf(f, DATE "%s\n", date);
	if ((s = getenv("REMOTE_ADDR")) != NULL)
		fprintf(f, IP "%s\n", s);
	if (!EMPTYSTRING(web))
		fprintf(f, WEB "%s\n", web);
	fputc('\n', f);
	/* escape "From " in the comment body */
	for (buf = text; (s = strstr(buf, FROM_)) != NULL; buf = s+1) {
		if (s == text) {
			fputs(">F", f);
		} else {
			c = *s;	
			*s = '\0';
			fputs(buf, f);
			*s = c;
			for (p = s-1; p >= text && *p == '>'; --p);
			if (p >= text && (*p == '\n' || *p == '\r'))
				fputc('>', f);
			fputc('F', f);
		}

	}
	fputs(buf, f);
	fputs("\n\n", f);
	/* unlock */
	fl.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &fl);
	fclose(f);
	return 0;

err2:	/* unlock */
	fl.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &fl);
err:	if (f != NULL)
		fclose(f);
	return -1;
}

int
post_comment(const char *article)
{
	char buf[MAX_COMMENT_LEN+1];
	char *author, *mail, *web, *text;
	const char *errstr;
	size_t len;
	extern char *error_str;
	extern struct query *query_post;

	assert(!EMPTYSTRING(article));
	if (query_post == NULL) {
		len = strtonum(getenv("CONTENT_LENGTH"), 0, LONG_MAX, &errstr);
		if (errstr != NULL) {
			error_str = ERR_COMMENT_FORM_READ;
			return -1;
		}
		if (len > MAX_COMMENT_LEN) {
			error_str = ERR_COMMENT_FORM_LEN;
			return -1;
		}
		if (fread(buf, len, 1, stdin) == 0 && !feof(stdin)) {
			error_str = ERR_COMMENT_FORM_READ;
			return -1; 
		}
		buf[len] = '\0';
		if ((query_post = tokenize_query(buf)) == NULL) {
			error_str = ERR_COMMENT_FORM_READ;
			return -1;
		}
	}
	author = get_query_param(query_post, "author");
	strchomp(author);
	mail = get_query_param(query_post, "mail");
	strchomp(mail);
	web = get_query_param(query_post, "web");
	strchomp(web);
	text = get_query_param(query_post, "text");
	strchomp(text);
	if (EMPTYSTRING(author)) {
		error_str = ERR_COMMENT_FORM_AUTHOR;
		return -1;
	}
	if (EMPTYSTRING(text)) {
		error_str = ERR_COMMENT_FORM_TEXT;
		return -1;
	}
	if (!antispam_verify(article,
	    get_query_param(query_post, "antispam_result"),
	    get_query_param(query_post, "antispam_hash"))) {
		error_str = ERR_COMMENT_FORM_ANTISPAM;
		return -1;
	}
	if (write_comment(article, author, mail, web, text) == -1) {
		error_str = ERR_COMMENT_FORM_WRITE;
		return -1;
	}
	return 0;
}

int
are_comments_readable(const char *article)
{
	FILE *f;

	if ((f = open_comments_file(article, MODE_READ)) == NULL)
		return 0;
	fclose(f);
	return 1;
}

unsigned long
read_comments(const char *article, comment_cb *callback)
{
	FILE *f;
	char *buf, *s;
	size_t len;
	ulong nb_comments;
	struct comment c;

	assert(article != NULL && *article != '\0');
	if ((f = open_comments_file(article, MODE_READ)) == NULL)
		return 0;
	nb_comments = 0;
	while (!feof(f)) {
		c.author = c.ip = c.mail = c.web = NULL;
		c.date = (time_t)-1;
		c.body_read = 0;
		/* parse header */
		while ((buf = fgetln(f, &len)) && len > 0 && *buf != '\n') {
			if (buf[len - 1] == '\n')
				buf[--len] = '\0';
			else
				/* EOF without EOL, ignore incomplete comment */
				goto next;
			if (c.author == NULL && len > sizeof(AUTHOR)
			    && strncmp(buf, AUTHOR, sizeof(AUTHOR)-1) == 0) {
				c.author = strdup(buf+sizeof(AUTHOR)-1);
				if (c.author == NULL) {
					warn("strdup");
					goto next;
				}
				/* extract email */
				if (c.author[len-sizeof(AUTHOR)] == '>') {
					for (s = c.author + len-sizeof(AUTHOR)-1;
					    s >= c.author && *s != '<'; s--);
					if (s != c.author && isspace(*(s-1))) {
						c.author[len-sizeof(AUTHOR)] = '\0';
						*s = '\0';
						c.mail = s+1;
					}
				}
			}
			if (c.date == (time_t)-1 && len > sizeof(DATE)
			    && strncmp(buf, DATE, sizeof(DATE)-1) == 0)
				c.date = rfc822_date(buf+sizeof(DATE)-1);
			if (callback == NULL)
				continue;
			if (c.ip == NULL && len > sizeof(IP)
			    && strncmp(buf, IP, sizeof(IP)-1) == 0) {
				c.ip = strdup(buf+sizeof(IP)-1);
				if (c.ip == NULL) {
					warn("strdup");
					goto next;
				}
			}
			if (c.web == NULL && len > sizeof(WEB)
			    && strncmp(buf, WEB, sizeof(WEB)-1) == 0) {
				c.web = strdup(buf+sizeof(WEB)-1);
				if (c.web == NULL) {
					warn("strdup");
					goto next;
				}
			}
		}
		if (buf != NULL) {
			if (!EMPTYSTRING(c.author) && c.date != (time_t)-1) {
				++nb_comments;
				if (callback != NULL) {
					c.number = nb_comments;
					c.body = f;
					callback(&c);
				}
			}
			if (!c.body_read) {
				/* find the next comment header */
				while ((buf = fgetln(f, &len))
			    	&& (len < sizeof(FROM_)
			    	|| strncmp(buf, FROM_, sizeof(FROM_)-1) != 0));
			}
		}
next:		free(c.author);
		free(c.ip);
		free(c.web);
	}
	fclose(f);
	return nb_comments;
}

char *
read_commentln(struct comment *c, int *freeln)
{
	char *buf, *lbuf, *escape;
	size_t len;

	assert(c != NULL);
	assert(c->body != NULL);
	lbuf = NULL;
	*freeln = 0;
	/* the body cannot be read more than one time */
	if (c->body_read)
		return NULL;
	/* read one line of the comment */
	if ((buf = fgetln(c->body, &len)) == NULL)
		return NULL;
	if (buf[len - 1] == '\n')
		buf[--len] = '\0';
	else {
		/* EOF without EOL, copy and add the NUL */
		if ((lbuf = malloc(len)) == NULL) {
			warn("malloc");
			return NULL;
		}
		memcpy(lbuf, buf, len);
		lbuf[len] = '\0';
		buf = lbuf;
		*freeln = 1;
	}
	if (len > sizeof(FROM_) && strncmp(buf, FROM_, sizeof(FROM_)-1) == 0) {
		/* the next comment header has been reached */
		c->body_read = 1;
		if (lbuf != NULL) {
			free(lbuf);
			*freeln = 0;
		}
		return NULL;
	}
	escape = buf;
	while (*escape == '>')
		++escape;
	if (strncmp(FROM_, escape, sizeof(FROM_)-1) == 0)
		++buf;
	return buf;
}
