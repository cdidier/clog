clog
====

## About ##

clog is a small, fast and well featured web log (blog) engine for those
who like the command line and are not afraid of the C language. It runs
as a CGI binary.

Main features:
 - Dynamic or static page rendering
 - Use templates to design pages
 - Create, edit and manage articles using your favourite tools
 (usually a text-based editor, e.g. vi)
 - Comments stored in a mbox-like file (can be managed using a
 mail client, e.g. mutt or even mail)
 - Simple anti-spam to protect comment posting
 - Tags support and tag cloud
 - RSS feeds (the main feed and a feed per tag)
 - GZip compression

Written in C and released under the ISC license.

## Installation ##

If your are using this blog engine on Linux (or if it doesn't build
on others unix-likes system), you must edit the Makefile and
uncomment the `COMPAT_SRCS` line.

Edit the file `config.h` to configure the blog engine before compiling it.
Edit the file `locale.h` if you want to translate the messages.

Then to build it, just type:
```
	$ make
```

Now you can copy (or link) the binary into your cgi-bin directory of the
http server.

## File layout ##

By default, the articles are stored in the directory `ARTICLES_DIR`
(defined in `config.h`). In this directory every articles are put in their
respective directory - named using the following format `YYYYMMDDHHmm`
(this is the date of the article, you can append an arbitrary string if
you want) - in a file named `article`. The first line of the file is the
title of the article and the rest is the content formatted in HTML. So
articles should look like that:
```
	ARTICLES_DIR/YYYYMMDDHHmm/article
```
For example:
```
	ARTICLES_DIR/200811121418/article
	ARTICLES_DIR/200811151212-my_nice_article/article
```

It is possible to write the continuation of the article (which will not
be displayed in the index and in the RSS feed) in the file `more` in
the associated article directory.

The comments are stored in the file `comments` in the associated article
directory. This file must exists and have the proper permissions (the
file must be writable by the http server user) to be able to post comments.
To manage the comments you can use a mail client like this basic one:
```
	$ mail -f path/to/comments
```

So a typical file layout for an article can be like this:
```
	ARTICLES_DIR/YYYYMMDDHHmm/article
	ARTICLES_DIR/YYYYMMDDHHmm/comments
	ARTICLES_DIR/YYYYMMDDHHmm/more
```

By default, the tags are stored in the directory `TAGS_DIR` (defined in
`config.h`). In this directory a tag is a directory - named after the name
of the tag - that contains files named after the directory name of the
articles (e.g. empty files or symbolic links). So tags should look like
that:
```
	TAGS_DIR/<tag name>/<article>
```
For example:
```
	TAGS_DIR/life/200811121418
	TAGS_DIR/life/200811151212-my_nice_article
	TAGS_DIR/geek/200811151212-my_nice_article
```

## Customising the templates ##

In the directory `TEMPLATES_DIR` (defined in `config.h`), there is some HTML
files which are only parts of a page. The blog engine will put together
these templates and insert the data to design the pages. In order to
know what to put where, the blog engine parses the templates to find
markers, which look like that `%%MARKER_NAME%%`.

There is some markers that can be inserted in every templates:
```
	%%BASE_URL%%	The URL of the blog
	%%BIN_URL%%	The URL of the blog binary
	%%SITE_NAME%%	The name of the blog
	%%DESCRIPTION%%	The description of the blog
	%%COPYRIGHT%%	The copyright notice
	%%CHARSET%%	The charset of the page
	%%HEADERS%%	Some additional HTML headers
```

The file `TEMPLATES_DIR/page.html` contains the general design of the page.
It can contains the following markers:
```
	%%PAGE_BODY%%	Contains the body of the page (indexes, articles, ...)
	%%PAGE_TITLE%%	The name of the page (prepended with " - ")
	%%NAVIGATION_NEXT%%	A link to the next page
	%%NAVIGATION_PREVIOUS%%	A link to the previous page
	%%NAVIGATION_PAGE%%	The current page number
	%%NAVIGATION_PAGES%%	The number of pages
```

The file `TEMPLATES_DIR/article.html` contains an article. It can
contains the following markers:
```
	%%ARTICLE_URL%%		The URL that points to the article
	%%ARTICLE_TITLE%%	The title of the article
	%%ARTICLE_DATE%%	The date of the article
	%%ARTICLE_TAGS%%	The links to the tags of the article
	%%ARTICLE_BODY%%	The actual content of the article
	%%ARTICLE_COMMENTS_INFO%%
	%%ARTICLE_COMMENTS%%
```

The file `TEMPLATES_DIR/article_comment.html` contains a comment. It can
contains the following markers:
```
	%%COMMENT_AUTHOR%%	The name of the author of the comment
	%%COMMENT_NB%%		The number of the comment
	%%COMMENT_MAIL%%	The mail address of the author
	%%COMMENT_WEB%%		The web page of the author
	%%COMMENT_DATE%%	The date of the comment
	%%COMMENT_TEXT%%	The actual content of the comment
```

The file `TEMPLATES_DIR/comment_form.html` contains the form to post a
comment. It can contains the following markers:
```
	%%FORM_POST_URL%%	The URL of the blog binary to post the comment
	%%ANTISPAM_JAM1%%	The first operand of the anti-spam
	%%ANTISPAM_JAM2%%	The second operand of the anti-spam
	%%ANTISPAM_HASH%%	An hidden field used by the anti-spam
	%%FORM_ERROR%%		An error message
	%%FORM_NAME%%
	%%FORM_MAIL%%
	%%FORM_WEB%%
	%%FORM_TEXT%%
```

## Using static renderer ##

You need to type some commands (described below) to generate the static
pages of your blog. The blog engine will automatically update them when
a comment is posted. To use the static mode by default, just define
`DEFAULT_STATIC` in the `config.h` file.

__/!\\__ Permissions consideration: The HTML and XML files in the directory
`BASE_DIR` (in `config.h`) must be writable by the http server user, to update
them when a comment is posted.

__/!\\__ Chroot considerations: If you run your http server in a chroot, the
root directory won't be the same if the blog engine is started by the
http server or from the command line, so the templates and data files
won't be at the same place. To fix this just modify `CHROOT_DIR` (in
`config.h`).

To generate all the static pages (you usually need this command when you
have added or deleted an article, if you have created a new tag or added
a new tag to an existing article):
```
	./blog -s all
```

To generate a specific article (if you have updated it):
```
	./blog -s <name of the directory of the article>
```
For example:
```
	./blog -s 200811121418a
```

You can also generate a specific page, like all the articles:
```
	./blog -s articles
```
the tag cloud:
```
	./blog -s tags
```
and the RSS feeds:
```
	./blog -s rss
```
