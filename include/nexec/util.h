#if !defined(NEXEC_UTIL_H_INCLUDED)
#define NEXEC_UTIL_H_INCLUDED

#include <openssl/ssl.h>

struct env {
	struct env	*next;
	const char	*name;
	const char	*value;
};

struct env	*alloc_env_or_die();
void		read_line(SSL *, char *, size_t);
void		set_tcp_nodelay_or_die(int);
void		writeln(SSL *, const char *);
void		sslutil_handle_error(SSL *, int, const char *);

#endif
