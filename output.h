/* $Id$ */

#ifndef RENDER_TOOLS_H
#define RENDER_TOOLS_H

typedef void (parse_markers_cb)(const char *, void *);

void	 hputc(const char);
void	 hputs(const char *);
void	 hputd(const long long);
void	 hput_escaped(const char *);
void	 hput_url(const char *, const char *);
void	 hput_pagelink(const char *, long page, const char *);
void	 parse_template(const char *, parse_markers_cb, void *);

#endif
