#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include <nexec/util.h>

static void
die_if_timeout(time_t t0)
{
    if (time(NULL) - t0 < 60) {
        return;
    }
    die("timeout");
}

static char
read_char(int fd)
{
    time_t t0 = time(NULL);

    ssize_t n;
    char c;
    while (((n = read(fd, &c, sizeof(c))) == -1) && (errno == EAGAIN)) {
        die_if_timeout(t0);

        struct timespec rqtp;
        rqtp.tv_sec = 0;
        rqtp.tv_nsec = 1000000;
        nanosleep(&rqtp, NULL);
    }
    if (n == -1) {
        die("cannot read a next char: %s", strerror(errno));
    }

    return c;
}

void
read_line(int fd, char* buf, size_t bufsize)
{
    char* pend = buf + bufsize;
    char* p = buf;
    char c;
    while ((p < pend) && (c = read_char(fd)) != '\r') {
        *p = c;
        p++;
    }
    if (p == pend) {
        p[-1] = '\0';
        die("too long request: %s", buf);
    }

    if (read_char(fd) != '\n') {
        die("invalid line terminator.");
    }

    *p = '\0';
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
