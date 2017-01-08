#include <sys/select.h>

#include <fsyscall/private/die.h>
#include <openssl/ssl.h>

#include <nexec/util.h>

static void
initialize_fds(int fd, fd_set* fds)
{

    FD_ZERO(fds);
    FD_SET(fd, fds);
}

static void
wait_io(int maxfd, fd_set* rfds, fd_set* wfds)
{
    struct timeval timeout;

    timeout.tv_sec = 120;
    timeout.tv_usec = 0;
    switch (select(maxfd + 1, rfds, wfds, NULL, &timeout)) {
    case -1:
        die(1, "select(2) failed");
    case 0:
        diex(1, "SSL connection timeouted");
    case 1:
        break;
    default:
        die(255, "unexpected behavior of select(2): %s:%u", __FILE__, __LINE__);
    }
}

void
sslutil_handle_error(SSL* ssl, int ret, const char* desc)
{
    int fd = SSL_get_fd(ssl);
    if (fd == -1) {
        die(1, "SSL_get_fd(3) failed");
    }

    int error = SSL_get_error(ssl, ret);
    const char *because;
    fd_set fds;
    switch (error) {
    case SSL_ERROR_NONE:
        because = "SSL_ERROR_NONE";
        break;
    case SSL_ERROR_SSL:
        because = "SSL_ERROR_SSL";
        break;
    case SSL_ERROR_WANT_READ:
        initialize_fds(fd, &fds);
        wait_io(fd, &fds, NULL);
        return;
    case SSL_ERROR_WANT_WRITE:
        initialize_fds(fd, &fds);
        wait_io(fd, NULL, &fds);
        return;
    case SSL_ERROR_WANT_X509_LOOKUP:
        because = "SSL_ERROR_WANT_X509_LOOKUP";
        break;
    case SSL_ERROR_SYSCALL:
        because = "SSL_ERROR_SYSCALL";
        break;
    case SSL_ERROR_ZERO_RETURN:
        diex(1, "the peer is shutdown (SSL_ERROR_ZERO_RETURN)");
    case SSL_ERROR_WANT_CONNECT:
        because = "SSL_ERROR_WANT_CONNECT";
        break;
    case SSL_ERROR_WANT_ACCEPT:
        because = "SSL_ERROR_WANT_ACCEPT";
        break;
    default:
        because = "unknown error";
        break;
    }

    die(1, "%s: SSL error: %s", desc, because);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
