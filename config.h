/* $Id$ */

/* The URL of the blog base directory (very useful when using the static
 * mode) */
#define	BASE_URL	"http://cybione.org/~cdidier/log/"
/* The URL of compiled binary of the blog engine */
#define BIN_URL		"http://cybione.org/~cdidier/cgi-bin/blog"

#define SITE_NAME	"C'log"
#define DESCRIPTION	"Colin Didier's web journal."
#define COPYRIGHT	"Copyright &#169; 2008 <a href=\"mailto:cdidier+clog@cybione.org\">Colin Didier</a>; All right reserved/Tous droits r&eacute;serv&eacute;s."
#define CHARSET		"UTF-8"
#define TITLE_SEPARATOR	" - "

#ifdef ENABLE_STATIC
#define CHROOT_DIR		"/var/www"
#endif /* ENABLE_STATIC */

/* Where the static pages are written */
#define BASE_DIR	"/users/cdidier/log"
/* Where the templates are stored */
#define TEMPLATES_DIR	BASE_DIR"/src/templates"
/* Where the articles are stored */
#define ARTICLES_DIR	BASE_DIR"/articles"
/* Where the tags are stored */
#define TAGS_DIR	BASE_DIR"/tags"

/* Number of articles per page (and also per RSS feed) */
#define NB_ARTICLES	5
/* Factor used to render the tag cloud (you should adjust it according
 * to the number of article) */
#define TAG_CLOUD_THRES	10

/* The format string of the date displayed in the pages (take a look at the
 * man page strftime(3) for the format) */
#define TIME_FORMAT	"%Y-%m-%d %R"
/* The maximum number of characters of the formatted date */
#define TIME_LENGTH	16

/* Parameters of the anti-spam */
#define JAM_MIN		1
#define	JAM_MAX		9
#define JAM_SALT	"ec4fec4358fab0886f04fbadeea18fb77f4359e8"
