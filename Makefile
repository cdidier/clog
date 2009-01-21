# $Id$

# Uncomment to display comments in the articl pages
CFLAGS+=-DENABLE_COMMENTS

# Uncomment to allow visitors to post comments
CFLAGS+=-DENABLE_POST_COMMENT

# Uncomment to enable the GZip compression of the HTTP protocol
CFLAGS+=-DENABLE_GZIP
LDFLAGS+=-lz

# Uncomment to enable the static mode
CFLAGS+=-DENABLE_STATIC

# Uncomment to build on Linux
#CFLAGS+=-DLINUX
#SRCS+=openbsd-compat/sha1.c openbsd-compat/strlcpy.c openbsd-compat/strtonum.c

### Usually you don't need to edit the following

PROG=blog
CFLAGS+=-W -Wall -Wpointer-arith -Wbad-function-cast -g
SRCS+= main.c post.c read.c render.c render_tools.c static.c
OBJS= ${SRCS:.c=.o}

all: ${PROG}

.c.o:
	${CC} ${CFLAGS} -o $@ -c $<

${PROG}: ${OBJS}
	${CC} ${LDFLAGS} -o ${PROG} ${OBJS}

clean:
	rm -f ${PROG} ${OBJS}
