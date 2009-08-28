/* $Id$ */

#ifndef RENDER_TOOLS_H
#define RENDER_TOOLS_H

typedef void (markers_cb)(const char *, void *);

void	open_output(const char *);
void	close_output(void);
void	document_begin_redirection(void);
void	document_end_redirection(void);
void	document_not_found(void);

void	hputc(const char);
void	hputs(const char *);
void	hputd(const long long);
void	hput_escaped(const char *);

/*
 * format:
 *     "article", "<article_name>"
 *     "rss", "<tag>"
 *     "tag", "<tag>", <page_number>, <articles_per_page>
 */
void	hput_url(char *, ...);

void	parse_template(const char *, markers_cb, void *);

#endif
