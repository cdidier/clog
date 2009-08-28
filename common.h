/* $Id$ */

#ifndef COMMON_H
#define COMMON_H

#include "config.h"
#include "locale.h"
#include "cgi.h"

#ifdef __linux__
#include "openbsd-compat/openbsd-compat.h"
#endif

#define EMPTYSTRING(x)	((x) == NULL || (*(x) == '\0'))

enum STATUS {
	STATUS_NONE		= 0,
	STATUS_FROMCMD		= 1<<0,
	STATUS_STATIC		= 1<<1,
	STATUS_POST		= 1<<2,
};

#endif
