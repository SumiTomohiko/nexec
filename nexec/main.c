#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <fsyscall/start_slave.h>

#include <nexec/config.h>

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

static void
usage()
{
    printf("Usage: %s hostname[:port] command...\n", getprogname());
}

static int
make_connected_socket(struct addrinfo* ai)
{
    int sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock == -1) {
        return sock;
    }
    if (connect(sock, ai->ai_addr, ai->ai_addrlen) != 0) {
        close(sock);
        return -1;
    }
    return sock;
}

static void
nexec_main(int argc, char* argv[])
{
    if (argc < 2) {
        usage();
        exit(1);
    }
    const char* s = argv[0];
    char buf[strlen(s) + 1];
    strcpy(buf, s);
    char* p = strrchr(buf, ':');
    if (p != NULL) {
        *p = '\0';
    }
    const char* hostname = buf;
    const char* servname = p != NULL ? &p[1] : NEXEC_DEFAULT_PORT;
    struct addrinfo hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo* ai;
    int ecode = getaddrinfo(hostname, servname, &hints, &ai);
    if (ecode != 0) {
        die("getaddrinfo() failed: %s", gai_strerror(ecode));
    }
    int sock = -1;
    struct addrinfo* res;
    for (res = ai; (res != NULL) && (sock == -1); res = res->ai_next) {
        sock = make_connected_socket(res);
    }
    freeaddrinfo(ai);
    if (sock == -1) {
        die("socket() failed.");
    }

    int wfd = dup(sock);
    if (wfd == -1) {
        err(1, "dup() failed");
    }
    /* TODO: Send argv. */
    fsyscall_start_slave(sock, wfd, argc - 1, argv + 1);
    /* NOTREACHED */
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

    nexec_main(argc - optind, argv + optind);

    return 0;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
