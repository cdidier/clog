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
#include <sha1.h>

#include "common.h"

#if defined(ENABLE_COMMENTS) && defined(ENABLE_POST_COMMENT)

#ifdef ENABLE_STATIC
void update_static_article(char *);
#endif /* ENABLE_STATIC */

void render_article(char *);
void render_page(page_cb, char *);
void redirect(char *);

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
create_comments_dir(char *aname)
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
write_comment(char *aname, struct cform *cf)
{
	struct stat st;
	FILE *fout;
	char path[MAXPATHLEN], cname[FILE_MINLEN+1], *end;
	time_t now;
	struct tm tm;

	/* The article exists ? */
	snprintf(path, MAXPATHLEN, ARTICLES_DIR"/%s/article", aname);
	if (stat(path, &st) != 0)
		redirect(aname);
	time(&now);
	localtime_r(&now, &tm);
	strftime(cname, FILE_MINLEN+1, FILE_FORMAT, &tm);
	snprintf(path, MAXPATHLEN, ARTICLES_DIR"/%s/comments/%sa", aname,
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
		if (create_comments_dir(aname) == -1)
			goto err;
		if ((fout = fopen(path, "w")) == NULL)
			goto err;
	}
	fprintf(fout, "%s%s\n", COMMENT_AUTHOR, cf->name);
	fprintf(fout, "%s%s\n", COMMENT_IP, getenv("REMOTE_ADDR"));
	if (cf->mail != NULL && *cf->mail != '\0')
		fprintf(fout, "%s%s\n", COMMENT_MAIL, cf->mail);
	if (cf->web != NULL && *cf->web != '\0')
		fprintf(fout, "%s%s\n", COMMENT_WEB, cf->web);
	fputc('\n', fout);
	fputs(cf->text, fout);
	fclose(fout);
#ifdef ENABLE_STATIC
	memset(cf, 0, sizeof(struct cform));
	update_static_article(aname);
#endif /* ENABLE_STATIC */
	redirect(aname); 
	return;
err:
	cf->error = ERR_CFORM_WRITE;
	render_page(render_article, aname);
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
post_comment(char *aname)
{
	char buf[MAX_INPUT_LEN];
	size_t len;
	const char *errstr;
	char *s, *sep, *jam, *jam_hash, *nl;
	extern struct cform comment_form;

	len = strtonum(getenv("CONTENT_LENGTH"), 0, LONG_MAX, &errstr);
	if (errstr != NULL) {
		warn("strtonum");
		goto out;
	}
	if (len > MAX_INPUT_LEN) {
		comment_form.error = ERR_CFORM_LEN;
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
			comment_form.name = s+sizeof(INPUT_NAME)-1;
			comment_form.name[sizeof(INPUT_NAME)+INPUT_LEN]
			    = '\0';
			if ((nl = strchr(comment_form.name, '\n')) != NULL)
				*nl = '\0';
		} else if (strncmp(s, INPUT_MAIL, sizeof(INPUT_MAIL)-1) == 0) {
			comment_form.mail = s+sizeof(INPUT_MAIL)-1;
			comment_form.mail[sizeof(INPUT_MAIL)+INPUT_LEN]
			    = '\0';
			if ((nl = strchr(comment_form.name, '\n')) != NULL)
				*nl = '\0';
		} else if (strncmp(s, INPUT_WEB, sizeof(INPUT_WEB)-1) == 0) {
			comment_form.web = s+sizeof(INPUT_WEB)-1;
			comment_form.web[sizeof(INPUT_WEB)+INPUT_LEN]
			    = '\0';
			if ((nl = strchr(comment_form.web, '\n')) != NULL)
				*nl = '\0';
		} else if (strncmp(s, INPUT_TEXT, sizeof(INPUT_TEXT)-1) == 0)
			comment_form.text = s+sizeof(INPUT_TEXT)-1;
	} 
	if (comment_form.name == NULL || *comment_form.name == '\0')
		comment_form.error = ERR_CFORM_NAME;
	else if (comment_form.text == NULL || *comment_form.text == '\0')
		comment_form.error = ERR_CFORM_TEXT;
	else if (!verify_jam(jam, jam_hash))
		comment_form.error = ERR_CFORM_JAM;
	else {
		write_comment(aname, &comment_form);
		return;
	}
out:	render_page(render_article, aname);
}

#endif /* ENABLE_POST_COMMENT */
