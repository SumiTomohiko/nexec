#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <nexec/util.h>

void
set_tcp_nodelay_or_die(int sock)
{
    int optval = 1;
    socklen_t optlen = sizeof(optval);
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &optval, optlen) != 0) {
        die("setsockopt(2) to set TCP_NODELAY failed: %s", strerror(errno));
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
