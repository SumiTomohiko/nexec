#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>

#include <fsyscall/private/die.h>

#include <nexec/util.h>

void
set_tcp_nodelay_or_die(int sock)
{
	socklen_t optlen;
	int optval;

	optval = 1;
	optlen = sizeof(optval);
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &optval, optlen) != 0)
		die(1, "setsockopt(2) to set TCP_NODELAY failed");
}
