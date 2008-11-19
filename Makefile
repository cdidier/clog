# $Id$

PROG= blog.cgi
CFLAGS=-W -Wall
LDFLAGS=-lz
SRCS= main.c post.c read.c render.c

.include <bsd.prog.mk>
