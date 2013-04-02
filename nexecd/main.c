#include <assert.h>
#include <err.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fsyscall/start_master.h>

#include <nexec/config.h>

/**
 * TODO: Share this function with nexec.
 */
static void
die(const char* fmt, ...)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s\n", fmt);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, buf, ap);
    va_end(ap);

    exit(1);
}

static int
make_bound_socket(struct addrinfo* ai)
{
    int sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock == -1) {
        return sock;
    }
    int on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
        err(1, "setsockopt() failed");
    }
    if (bind(sock, ai->ai_addr, ai->ai_addrlen) != 0) {
        warn("bind() failed");
        close(sock);
        return -1;
    }
    return sock;
}

static void
nexecd_main()
{
    struct addrinfo hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo* ai;
    int ecode = getaddrinfo(NULL, "57005", &hints, &ai);
    if (ecode != 0) {
        die("getaddrinfo() failed: %s", gai_strerror(ecode));
    }
    int sock = -1;
    struct addrinfo* res;
    for (res = ai; (res != NULL) && (sock == -1); res = res->ai_next) {
        sock = make_bound_socket(res);
    }
    freeaddrinfo(ai);
    if (sock == -1) {
        die("Cannot bind.");
    }
    if (listen(sock, 0) != 0) {
        err(1, "listen() failed");
    }

    if (daemon(0, 0) != 0) {
        err(1, "daemon() failed");
    }

    int fd;
    while ((fd = accept(sock, NULL, 0)) != -1) {
        pid_t pid = fork();
        assert(pid != -1);
        int status;
        int wfd;
        char* args[] = { "/bin/echo", "foo" };
        switch (pid) {
        case 0:
            wfd = dup(fd);
            if (wfd == -1) {
                err(1, "dup() failed");
            }
#if 0
            fsyscall_start_master(fd, wfd, argc - 1, argv + 1);
#endif
            fsyscall_start_master(fd, wfd, 2, args);
            /* NOTREACHED */
            break;
        default:
            close(fd);
            /* TODO: Do not wait child's termination. */
            waitpid(pid, &status, 0);
            break;
        }
    }
    close(sock);
}

int
main(int argc, char* argv[])
{
    struct option opts[] = {
        { "version", no_argument, NULL, 'v' },
        { NULL, 0, NULL, 0 } };
    int opt;
    while ((opt = getopt_long(argc, argv, "v", opts, NULL)) != -1) {
        switch (opt) {
        case 'v':
            printf("%s %s\n", getprogname(), NEXEC_VERSION);
            return 0;
        default:
            break;
        }
    }

    nexecd_main();

    return 0;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
