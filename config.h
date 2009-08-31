/* $Id$ */

/* Define DEFAULT_STATIC if you want the blog engine to work in static mode
 * by default. Accessing the binary will redirect the visitor to the static
 * pages, posting a comment will regenerate the static files of the article...
 */
#define DEFAULT_STATIC

/* The URL of the binary of the blog engine */
#define BIN_URL		"http://cybione.org/~cdidier/cgi-bin/blog"
/* The URL of the blog base directory */
#define	BASE_URL	"http://cybione.org/~cdidier/blog/"

#define SITE_NAME	"C'log"
#define DESCRIPTION	"Colin Didier's web journal."
#define CHARSET		"UTF-8"
#define COPYRIGHT	"Copyright &#169; 2008-2009 <a href=\"mailto:cdidier+clog@cybione.org\">Colin Didier</a>; All right reserved/Tous droits r&eacute;serv&eacute;s."

/* This is the absolute path of the directory where the http server is chrooted
 * (leave empty if not chrooted) */
#define CHROOT_DIR	"/var/www"
/* Where the static pages are written */
#define BASE_DIR	"/users/cdidier/blog"
/* Where the templates are stored */
#define TEMPLATES_DIR	BASE_DIR"/src/templates"
/* Where the articles are stored */
#define ARTICLES_DIR	BASE_DIR"/articles"
/* Where the tags are stored */
#define TAGS_DIR	BASE_DIR"/tags"

/* Number of articles per page (and also per RSS feed) */
#define NB_ARTICLES	5

/* The format string of the date displayed in the pages (take a look at the
 * man page strftime(3) for the format) */
#define TIME_FORMAT	"%Y-%m-%d %R"

/* Parameter of the antispam, you must change it with an other random value */
#define ANTISPAM_JAM_SALT	"ec4fec4358fab0886f04fbadeea18fb77f4359e8"
