/* $Id$ */

#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <time.h>

#include "config.h"
#include "locale.h"

#ifdef LINUX
#include "openbsd-compat/openbsd-compat.h"
#endif

#define COMMENT_MAXLEN	51
#define COMMENT_AUTHOR	"Author: "
#define COMMENT_IP	"Ip: "
#define COMMENT_MAIL	"Mail: "
#define COMMENT_WEB	"Web: "

/* file name format: YYYYMMDDHHmm... */
#define FILE_FORMAT "%Y%m%d%H%M"
#define FILE_MINLEN 12

typedef void (page_cb)(const char *);
typedef void (article_cb)(const char *, const char *, const struct tm *,
    FILE *, uint);
typedef void (article_tag_cb)(const char *);
typedef void (tag_cb)(const char *, ulong);
#ifdef ENABLE_COMMENTS
typedef void (comment_cb)(const char *, const struct tm *, const char *,
    const char *, const char *, FILE *);
#endif /* ENABLE_COMMENTS */

typedef int (foreach_comment_cb)(const char *, const char *, void *);
typedef int (foreach_article_cb)(const char *, void *);
typedef int (foreach_tag_cb)(const char *, void *);

enum {
	PAGE_UNKNOWN,
	PAGE_INDEX,
	PAGE_ARTICLE,
	PAGE_RSS,
	PAGE_TAG_CLOUD,
};

struct page {
	int type;
	union {
		struct p_index {
			const char	*tag;
			ulong		 page;
			ulong		 nb_articles;
		} i;
		struct p_article {
			const char	*name;
#if defined(ENABLE_COMMENTS) && defined(ENABLE_POST_COMMENT)
			char		*cform_name;
			char		*cform_mail;
			char		*cform_web;
			char		*cform_text;
			char		*cform_error;
#endif /* ENABLE_POST_COMMENT */
		} a;
	};
};

#endif
