/* $Id$ */

#ifndef ARTICLE_H
#define ARTICLE_H

#include <stdio.h>
#include <time.h>
#include <sys/queue.h>
#include "antispam.h"

struct article_tag {
	char		*name;
	unsigned long	 number;
	SLIST_ENTRY(article_tag) next;
};

struct article {
	const char	*name;
	char		*title;
	struct tm	 date;
	SLIST_HEAD(, article_tag) tags;
	FILE		*body;
	FILE		*more;
	off_t		 more_size;
	char		 display_more;
	struct antispam	*antispam;
};

struct tag {
	const char	*name;
	unsigned long	 page, offset, number, pages; 
	char		 previous, next;
};

typedef void (article_cb)(struct article *);
typedef void (tag_cb)(struct tag *);

int			 is_article_name(const char *, size_t);
int			 read_article(const char *, article_cb *);
struct article_tag	*get_article_tags(const char *);
unsigned long		 read_articles(const char *, unsigned long,
			     unsigned long, article_cb *);
int			 read_tag(const char *, unsigned long,
			     unsigned long, tag_cb);

#endif
