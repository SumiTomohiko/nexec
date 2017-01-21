#if !defined(NEXEC_NEXECD_H_INCLUDED)
#define NEXEC_NEXECD_H_INCLUDED

#include <sys/param.h>
#include <openssl/ssl.h>

struct ssl {
	SSL_CTX	*ctx;
};

struct daemon {
	char	user[16];
	char	group[16];
};

struct mapping {
	struct mapping	*next;
	char		name[32];
	char		path[MAXPATHLEN];
};

struct config {
	struct daemon	daemon;
	struct mapping	*mappings;
	struct ssl	ssl;
};

/* child_main.c */
void	child_main(struct config *, int);

/* memory.c */
void	*memory_allocate(size_t);
void	memory_dispose();
void	memory_initialize();

/* parser.y */
void	parser_initialize(struct config *);

/* cmdlexer.re */
struct cmdlexer_scanner;

enum command {
	CMD_LOGIN = 42,
	CMD_SET_ENV,
	CMD_EXEC
};

enum scanner_status {
	SCANNER_ERROR = 42,
	SCANNER_SUCCESS,
	SCANNER_END
};

struct cmdlexer_scanner	*cmdlexer_create_scanner();
void			cmdlexer_scan(struct cmdlexer_scanner *, const char *);
enum scanner_status	cmdlexer_next_command(struct cmdlexer_scanner *,
					      enum command *);
enum scanner_status	cmdlexer_next_string(struct cmdlexer_scanner *,
					     char **);

#endif
