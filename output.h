/* $Id$ */

#ifndef RENDER_TOOLS_H
#define RENDER_TOOLS_H

typedef void (parse_markers_cb)(const char *, void *);

void	 hputc(const char);
void	 hputs(const char *);
void	 hputd(const long long);
void	 hput_escaped(const char *);
void	 hput_url(int, const char *, ulong);
void	 hput_pagelink(int, const char *, ulong, const char *);
void	 parse_template(const char *, parse_markers_cb, void *);

#endif
