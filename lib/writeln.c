#include <sys/types.h>
#include <sys/uio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <fsyscall/private/die.h>
#include <openssl/ssl.h>

#include <nexec/util.h>

static void
write_all(SSL *ssl, const void *buf, size_t bufsize)
{
	size_t nbytes;
	ssize_t n;
	const void *p;

	nbytes = 0;
	while (nbytes < bufsize) {
		p = (void *)((uintptr_t)buf + nbytes);
		n = SSL_write(ssl, p, bufsize - nbytes);
		if (n < 0)
			die(1, "cannot write");
		nbytes += n;
	}
}

void
writeln(SSL *ssl, const char *msg)
{
	size_t bufsize;
	const char *newline;
	char *buf;

	syslog(LOG_DEBUG, "write: %s", msg);

	newline = "\r\n";
	bufsize = strlen(msg) + strlen(newline);
	buf = (char *)alloca(bufsize + 1);
	sprintf(buf, "%s%s", msg, newline);
	write_all(ssl, buf, bufsize);
}
