# $Id$

PROG= blog
CFLAGS=-W -Wall -g
LDFLAGS=-lz
SRCS= main.c post.c read.c render.c static.c

.include <bsd.prog.mk>
