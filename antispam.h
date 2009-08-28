#ifndef ANTISPAM_H
#define ANTISPAM_H

#ifdef __linux__
#include "openbsd-compat/sha1.h"
#else
#include <sha1.h>
#endif

#define ANTISPAM_JAM_MIN	1
#define ANTISPAM_JAM_MAX	9

struct antispam {
	unsigned int	jam1, jam2;
	char		hash[SHA1_DIGEST_STRING_LENGTH];
};

struct antispam	*antispam_generate(const char *);
int		 antispam_verify(const char *, const char *, const char *);

#endif
