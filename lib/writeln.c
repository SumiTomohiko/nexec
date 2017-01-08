#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <syslog.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include <fsyscall/private/die.h>
#include <nexec/util.h>

static void
write_all(SSL* ssl, const void* buf, size_t bufsize)
{
    size_t nbytes = 0;
    while (nbytes < bufsize) {
        const void* p = (void*)((uintptr_t)buf + nbytes);
        ssize_t n = SSL_write(ssl, p, bufsize - nbytes);
        if (n < 0) {
            die(1, "cannot write");
        }
        nbytes += n;
    }
}

void
writeln(SSL* ssl, const char* msg)
{
    syslog(LOG_DEBUG, "write: %s", msg);

    const char* newline = "\r\n";
    size_t bufsize = strlen(msg) + strlen(newline);
    char buf[bufsize + 1];
    sprintf(buf, "%s%s", msg, newline);
    write_all(ssl, buf, bufsize);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
