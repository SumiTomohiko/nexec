#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <nexec/util.h>

void
setblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
        die("fcntl() failed to be blocking: %s", strerror(errno));
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
