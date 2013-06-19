#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <nexec/util.h>

void
setnonblock(int fd)
{
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        die("fcntl() failed for F_SETFL and O_NONBCLOCK: %s", strerror(errno));
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
