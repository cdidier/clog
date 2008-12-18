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

#ifdef ENABLE_STATIC
#define MOD_FORMAT	"%a %b %e %H:%M:%S %Y"
#define MOD_BEGIN	"<!-- $ModDate:"
#define MOD_END		"$ !-->"
#endif /* ENABLE_STATIC */

typedef void (page_cb)(const char *);
typedef void (article_cb)(const char *, const char *, const struct tm *,
    FILE *, uint);
typedef void (article_tag_cb)(const char *);
typedef void (tag_cb)(const char *, uint);
#ifdef ENABLE_COMMENTS
typedef void (comment_cb)(const char *, const struct tm *, const char *,
    const char *, const char *, FILE *);
#endif /* ENABLE_COMMENTS */

typedef int (foreach_article_cb)(const char *, void *);

struct cform {
	char	*name;
	char	*mail;
	char	*web;
	char	*text;
	char	*error;
};

#endif
