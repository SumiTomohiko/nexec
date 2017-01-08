#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include <fsyscall/private/die.h>
#include <nexec/util.h>

static char
read_char(SSL* ssl)
{
    int n;
    char c;
    while ((n = SSL_read(ssl, &c, sizeof(c))) != sizeof(c)) {
        sslutil_handle_error(ssl, n, "SSL_read(3)");
    }

    return c;
}

void
read_line(SSL* ssl, char* buf, size_t bufsize)
{
    char* pend = buf + bufsize;
    char* p = buf;
    char c;
    while ((p < pend) && (c = read_char(ssl)) != '\r') {
        *p = c;
        p++;
    }
    if (p == pend) {
        p[-1] = '\0';
        die(1, "too long request: %s", buf);
    }

    if (read_char(ssl) != '\n') {
        die(1, "invalid line terminator.");
    }

    *p = '\0';
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
