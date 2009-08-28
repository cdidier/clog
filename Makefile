# $Id$

# OS compatibility (e.g. Linux)
#COMPAT_SRCS = openbsd-compat/sha1.c openbsd-compat/strlcpy.c openbsd-compat/strtonum.c

### Usually you don't need to edit the following

BIN = blog
DEBUG_CFLAGS = -g -W -Wall -Wpointer-arith -Wbad-function-cast
CFLAGS += ${DEBUG_CFLAGS}
LDFLAGS += -lz -static 
LIBS += /usr/lib/libz.a

SRCS += articles.c cgi.c comments.c main.c output.c render.c static.c tools.c ${COMPAT_SRCS}
OBJS = ${SRCS:.c=.o}

all: ${BIN}

.c.o:
	${CC} ${CFLAGS} -o $@ -c $<

${BIN}: ${OBJS}
	${CC} ${LDFLAGS} -o ${BIN} ${OBJS} ${LIBS}

clean:
	rm -f ${BIN} ${BIN}.core ${OBJS}
