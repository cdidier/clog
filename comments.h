/* $Id$ */

#ifndef COMMENTS_H
#define COMMENTS_H

#include <stdio.h>
#include <time.h>

struct comment {
	char	*author;
	time_t	 date;
	char	*ip;
	char	*mail;
	char	*web;
	unsigned long	 number;
	FILE	*body;
	char	 body_read;
};

typedef void (comment_cb)(struct comment *);

int		 are_comments_writable(const char *);
int		 post_comment(const char *);
int		 are_comments_readable(const char *);
unsigned long	 read_comments(const char *, comment_cb);
char		*read_commentln(struct comment *, int *);

#endif
