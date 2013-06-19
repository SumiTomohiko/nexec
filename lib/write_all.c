#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <syslog.h>
#include <unistd.h>

#include <nexec/util.h>

void
write_all(int fd, const void* buf, size_t bufsize)
{
    size_t nbytes = 0;
    while (nbytes < bufsize) {
        const void* p = (void*)((uintptr_t)buf + nbytes);
        ssize_t n = write(fd, p, bufsize - nbytes);
        if ((n == -1) && (errno != EAGAIN)) {
            die("cannot write: %s", strerror(errno));
        }
        nbytes += n;
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
