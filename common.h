/* $Id$ */

#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <time.h>

#include "config.h"
#include "locale.h"

#define COMMENT_MAXLEN	51
#define COMMENT_AUTHOR	"Author: "
#define COMMENT_IP	"Ip: "
#define COMMENT_MAIL	"Mail: "
#define COMMENT_WEB	"Web: "

/* file name format: YYYYMMDDHHmm... */
#define FILE_FORMAT "%Y%m%d%H%M"
#define FILE_MINLEN 12

typedef void (page_cb)(char *);
typedef void (article_cb)(char *, char *, struct tm *, FILE *, uint);
typedef void (article_tag_cb)(char *);
typedef void (comment_cb)(char *, struct tm *, char *, char *, char *, FILE *);
typedef void (tag_cb)(char *, uint);

struct cform {
	char	*name;
	char	*mail;
	char	*web;
	char	*text;
	char	*error;
};

#endif
