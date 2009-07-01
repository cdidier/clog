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

#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __linux__
#include "openbsd-compat/sha1.h"
#else
#include <sha1.h>
#endif

#include "common.h"

#if defined(ENABLE_COMMENTS) && defined(ENABLE_POST_COMMENT)

#ifdef ENABLE_STATIC
void update_static_article(const char *, int);
#endif /* ENABLE_STATIC */

void render_page();
void redirect(const char *);

#define INPUT_JAM	"jam="
#define INPUT_JAM_HASH	"jam_hash="
#define INPUT_NAME	"name="
#define INPUT_MAIL	"mail="
#define INPUT_WEB	"web="
#define INPUT_TEXT	"text="

#define FIELDS_LEN sizeof(INPUT_JAM)+sizeof(INPUT_JAM_HASH) \
	+sizeof(INPUT_NAME)+sizeof(INPUT_MAIL)+sizeof(INPUT_WEB) \
	+sizeof(INPUT_TEXT)
#define JAMS_LEN	SHA1_DIGEST_STRING_LENGTH+4
#define INPUT_LEN	101
#define INPUTS_LEN	3*INPUT_LEN
#define TEXT_LEN	2048

#define MAX_INPUT_LEN	FIELDS_LEN+JAMS_LEN+INPUTS_LEN+TEXT_LEN

static void
url_decode(char *src)
{
        static const char *hex = "0123456789abcdef";
	char *dest, *i;

	for(dest = src; *src != '\0'; src++, dest++) {
		if (*src == '%' && *++src != '\0'
		    && (i = strchr(hex, tolower(*src))) != NULL) {
			*dest = (i - hex) * 16;
			if (*++src != '\0'
			    && (i = strchr(hex, tolower(*src))) != NULL)
				*dest += i - hex;
		} else
			*dest = *src  == '+' ? ' ' : *src;
	}
	*dest = '\0';
}

static void
strchomp(char *s)
{
	size_t len, i, spaces;

	len = strlen(s);
	while (len > 0 && isspace(s[len-1]))
		s[--len] = '\0';
	for (spaces = 0; *s != '\0' && isspace(*s); ++s)
		++spaces;
	for (i = 0; i < len-spaces+1; ++i)
		s[i-spaces] = s[i];
}

static int
create_article_comments_dir(const char *aname)
{
	struct stat st;
	char path[MAXPATHLEN];

	snprintf(path, MAXPATHLEN, ARTICLES_DIR"/%s/comments", aname);
	if (stat(path, &st) == 0)
		return 0;
	if (mkdir(path, 0755) == -1) {
		warn("mkdir: %s", path);
		return -1;
	}
	return 0;
}

static void
write_article_comment(void)
{
	struct stat st;
	FILE *fout;
	char path[MAXPATHLEN], cname[FILE_MINLEN+1], *end;
	time_t now;
	struct tm tm;
	extern struct page globp;

	/* The article exists ? */
	snprintf(path, MAXPATHLEN, ARTICLES_DIR"/%s/article", globp.a.name);
	if (stat(path, &st) != 0)
		redirect(globp.a.name);
	time(&now);
	localtime_r(&now, &tm);
	strftime(cname, FILE_MINLEN+1, FILE_FORMAT, &tm);
	snprintf(path, MAXPATHLEN, ARTICLES_DIR"/%s/comments/%sa", globp.a.name,
	    cname);
	end = strchr(path, '\0')-1;
	while (end-path < MAXPATHLEN && stat(path, &st) == 0) {
		if (*end == 'z') {
			if (++end-path < MAXPATHLEN) {
				*end = 'a';
				end[1] = '\0';
			}
		} else
			++(*end);
	}
	if (end-path >= MAXPATHLEN)
		goto err;
	if ((fout = fopen(path, "w")) == NULL) {
		if (create_article_comments_dir(globp.a.name) == -1)
			goto err;
		if ((fout = fopen(path, "w")) == NULL)
			goto err;
	}
	fprintf(fout, "%s%s\n", COMMENT_AUTHOR, globp.a.cform_name);
	fprintf(fout, "%s%s\n", COMMENT_IP, getenv("REMOTE_ADDR"));
	if (globp.a.cform_mail != NULL && *globp.a.cform_mail != '\0')
		fprintf(fout, "%s%s\n", COMMENT_MAIL, globp.a.cform_mail);
	if (globp.a.cform_web != NULL && *globp.a.cform_web != '\0')
		fprintf(fout, "%s%s\n", COMMENT_WEB, globp.a.cform_web);
	fputc('\n', fout);
	fputs(globp.a.cform_text, fout);
	fclose(fout);
#ifdef ENABLE_STATIC
	globp.a.cform_name = globp.a.cform_mail = globp.a.cform_web
	    = globp.a.cform_text = globp.a.cform_error = NULL;
	update_static_article(globp.a.name, 0);
#endif /* ENABLE_STATIC */
	redirect(globp.a.name); 
	return;
err:
	globp.a.cform_error = ERR_CFORM_WRITE;
	render_page();
}

static int
verify_jam(const char *jam, const char *hash)
{
	char salt[sizeof(JAM_SALT)+1], hash2[SHA1_DIGEST_STRING_LENGTH];
	const char *errstr;
	int nb;
	
	if (hash == NULL || strlen(hash) != SHA1_DIGEST_STRING_LENGTH-1)
		return 0;
	nb = strtonum(jam, JAM_MIN, 2*JAM_MAX, &errstr);
	if (errstr != NULL)
		return 0;
	strlcpy(salt+1, JAM_SALT, sizeof(JAM_SALT));
	*salt = nb;
	SHA1Data(salt, sizeof(JAM_SALT), hash2);
	if (strcmp(hash, hash2) != 0)
		return 0;
	return 1;
}

void
post_article_comment(const char *aname)
{
	char buf[MAX_INPUT_LEN];
	size_t len;
	const char *errstr;
	char *s, *sep, *jam, *jam_hash, *nl;
	extern struct page globp;

	globp.type = PAGE_ARTICLE;
	globp.a.name = aname;
	len = strtonum(getenv("CONTENT_LENGTH"), 0, LONG_MAX, &errstr);
	if (errstr != NULL) {
		warn("strtonum");
		goto out;
	}
	if (len > MAX_INPUT_LEN) {
		globp.a.cform_error = ERR_CFORM_LEN;
		goto out;
	}
	if (fread(buf, len, 1, stdin) == 0 && !feof(stdin)) {
		warn("fread");
		goto out;
	}
	buf[len] = '\0';
	jam = jam_hash = NULL;
	for (s = buf; s != NULL && *s != '\0'; s = sep) {
		if ((sep = strchr(s, '&')) != NULL)
			*sep++ = '\0';
		strchomp(s);
		url_decode(s);
		if (strncmp(s, INPUT_JAM, sizeof(INPUT_JAM)-1) == 0)
			jam = s+sizeof(INPUT_JAM)-1;
		else if (strncmp(s, INPUT_JAM_HASH,
		    sizeof(INPUT_JAM_HASH)-1) == 0)
			jam_hash = s+sizeof(INPUT_JAM_HASH)-1;
		else if (strncmp(s, INPUT_NAME, sizeof(INPUT_NAME)-1) == 0) {
			globp.a.cform_name = s+sizeof(INPUT_NAME)-1;
			if ((nl = strchr(globp.a.cform_name, '\n')) != NULL)
				*nl = '\0';
		} else if (strncmp(s, INPUT_MAIL, sizeof(INPUT_MAIL)-1) == 0) {
			globp.a.cform_mail = s+sizeof(INPUT_MAIL)-1;
			if ((nl = strchr(globp.a.cform_mail, '\n')) != NULL)
				*nl = '\0';
		} else if (strncmp(s, INPUT_WEB, sizeof(INPUT_WEB)-1) == 0) {
			globp.a.cform_web = s+sizeof(INPUT_WEB)-1;
			if ((nl = strchr(globp.a.cform_web, '\n')) != NULL)
				*nl = '\0';
		} else if (strncmp(s, INPUT_TEXT, sizeof(INPUT_TEXT)-1) == 0)
			globp.a.cform_text = s+sizeof(INPUT_TEXT)-1;
	} 
	if (globp.a.cform_name == NULL || *globp.a.cform_name == '\0')
		globp.a.cform_error = ERR_CFORM_NAME;
	else if (globp.a.cform_text == NULL || *globp.a.cform_text == '\0')
		globp.a.cform_error = ERR_CFORM_TEXT;
	else if (!verify_jam(jam, jam_hash))
		globp.a.cform_error = ERR_CFORM_JAM;
	else {
		write_article_comment();
		return;
	}
out:	render_page();
}

#endif /* ENABLE_POST_COMMENT */
