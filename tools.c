/*
 * $Id$
 *
 * Copyright (c) 2009 Colin Didier <cdidier@cybione.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <assert.h>
#include <err.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "antispam.h"

void
sanitize_input(char *s)
{
	char *p;

	if ((p = strstr(s, "..")) != NULL)
		*p = '\0';
	if ((p = strchr(s, '/')) != NULL)
		*p = '\0';
}

void
strchomp(char *s)
{
	size_t len, i, spaces;

	len = strlen(s);
	/* remove spaces at the end of the string */
	while (len > 0 && isspace(s[len-1]))
		s[--len] = '\0';
	/* remove spaces at the beginning of the string */
	for (spaces = 0; *s != '\0' && isspace(*s); ++s)
		++spaces;
	for (i = 0; i < len-spaces+1; ++i)
		s[i-spaces] = s[i];
}

static long
parse_timezone_rfc822(const char *tz)
{
	const char *rfc822_timezones[26][4] = {
		{ "M", NULL },			/* UTC-12 */
		{ "L", NULL },
		{ "K", NULL },
		{ "I", NULL },
		{ "H", "PST", NULL }, 		/* UTC-8 */
		{ "G", "MST", "PDT", NULL },	/* UTC-7 */
		{ "F", "CST", "MDT", NULL },	/* UTC-6 */
		{ "E", "EST", "CDT", NULL },	/* UTC-5 */
		{ "D", "EDT", NULL },		/* UTC-4 */
		{ "C", NULL },
		{ "B", NULL },
		{ "A", NULL },
		{ "Z", "UT", "GMT", NULL },	/* UTC */
		{ "N", NULL },
		{ "O", NULL },
		{ "P", NULL },
		{ "Q", NULL },
		{ "R", NULL },
		{ "S", NULL },
		{ "T", NULL },
		{ "U", NULL },
		{ "V", NULL },
		{ "W", NULL },
		{ "X", NULL },
		{ "Y", NULL },			/* UTC+12 */
		{ NULL }
	};
	long i, j;
	size_t len;

	len = strlen(tz);
	if (strlen(tz) == 5 && (*tz == '+' || *tz == '-')
	    && isdigit(tz[1]) && isdigit(tz[2]) && isdigit(tz[3])
	    && isdigit(tz[4])) {
		i = atoi(tz);
		return ((i/100)*60 + i%100) * 60;
	}
	for (i = 0; rfc822_timezones[i] != NULL; ++i)
		for (j = 0; rfc822_timezones[i][j] != NULL; ++j)
			if (strcmp(rfc822_timezones[i][j], tz) == 0)
				return (i - 12) * 3600;
	return 0;
}

time_t
rfc822_date(char *date)
{
	struct tm tm, *lt;
	time_t now;
	char *p;
	int i;
	long offset;
	char *formats[] = { "%d %b %Y %T", "%d %b %Y %H:%M", "%d %b %y %T",
	     "%d %b %y %H:%M", NULL };

	memset(&tm, 0, sizeof(struct tm));
	if ((p = strchr(date, ',')) != NULL)
		date = p+2; /* ignore day of the week */
	strchomp(date);
	for (i = 0; formats[i] != NULL
	    && (p = strptime(date, formats[i], &tm)) == NULL; ++i);
	if (p == NULL)
		return (time_t)-1;
	strchomp(p);
	tm.tm_isdst = -1;
	tm.tm_gmtoff = offset = *p != '\0' ? parse_timezone_rfc822(p) : 0;
	/* get the current time to convert to the local timezone */
	time(&now);
	lt = localtime(&now);
	return mktime(&tm) - offset + lt->tm_gmtoff;
}

struct antispam *
antispam_generate(const char *additional_salt)
{
	struct antispam *as;
	char *data;
	size_t len;
	
	assert(additional_salt != NULL);
	srand(time(NULL));
	len = sizeof(ANTISPAM_JAM_SALT) + strlen(additional_salt) + 1;
	if ((data = malloc(len)) == NULL
	    || (as = malloc(sizeof(struct antispam))) == NULL) {
		warnx("malloc");
		return NULL;
	}
	as->jam1 = rand()%(ANTISPAM_JAM_MAX-ANTISPAM_JAM_MIN+1)
	    + ANTISPAM_JAM_MIN;
	as->jam2 = rand()%(ANTISPAM_JAM_MAX-ANTISPAM_JAM_MIN+1)
	    + ANTISPAM_JAM_MIN;
	strlcpy(data+1, ANTISPAM_JAM_SALT, len-1);
	strlcpy(data+sizeof(ANTISPAM_JAM_SALT), additional_salt,
	    len-sizeof(ANTISPAM_JAM_SALT));
	*data = (char)(as->jam1 + as->jam2);
	SHA1Data((u_int8_t *)data, len, as->hash);
	free(data);
	return as;
}

int
antispam_verify(const char *additional_salt, const char *result,
    const char *hash)
{
	char *data, hash2[SHA1_DIGEST_STRING_LENGTH];
	const char *errstr;
	size_t len;
	int nb;

	if (additional_salt == NULL || result == NULL || hash == NULL)
		return 0;
	nb = strtonum(result, ANTISPAM_JAM_MIN, 2*ANTISPAM_JAM_MAX, &errstr);
	if (errstr != NULL)
		 return 0;
	len = sizeof(ANTISPAM_JAM_SALT) + strlen(additional_salt) + 1;
	if ((data = malloc(len)) == NULL) {
		warnx("malloc");
		return 0;
	}
	strlcpy(data+1, ANTISPAM_JAM_SALT, len-1);
	strlcpy(data+sizeof(ANTISPAM_JAM_SALT), additional_salt,
	    len-sizeof(ANTISPAM_JAM_SALT));
	*data = (char)nb;
	SHA1Data((u_int8_t *)data, len, hash2);
	free(data);
	return strcmp(hash, hash2) == 0;
}
