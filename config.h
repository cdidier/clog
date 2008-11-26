/* $Id$ */

#define ENABLE_COMMENTS	
#define ENABLE_POST_COMMENT
#define ENABLE_GZIP
#define ENABLE_STATIC

#define	BASE_URL	"http://cybione.org/~cdidier/log/"
#define BIN_URL		"http://cybione.org/~cdidier/cgi-bin/blog"
#define SITE_NAME	"C'log"
#define DESCRIPTION	"Colin's web journal."
#define COPYRIGHT	"Copyright &#169; 2008 <a href=\"mailto:cdidier+clog@cybione.org\">Colin Didier</a>; Tous droits réservés."
#define CHARSET		"iso-8859-15"

#ifdef ENABLE_STATIC
#define CHROOT_DIR		"/var/www"
#define STATIC_EXTENSION	".html"
#endif /* ENABLE_STATIC */

#define BASE_DIR	"/users/cdidier/log"
#define TEMPLATES_DIR	BASE_DIR"/templates"
#define ARTICLES_DIR	BASE_DIR"/data/articles"
#define TAGS_DIR	BASE_DIR"/data/tags"

#define NB_ARTICLES	5
#define TAG_CLOUD_THRES	10

#define TIME_FORMAT	"%Y-%m-%d %R"
#define TIME_LENGTH	16

#define JAM_MIN		1
#define	JAM_MAX		9
#define JAM_SALT	"ec4fec4358fab0886f04fbadeea18fb77f4359e8"
