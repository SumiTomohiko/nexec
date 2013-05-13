#include <assert.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>
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
    printf("usage: %s hostname[:port] command...\n", getprogname());
}

static int
make_connected_socket(struct addrinfo* ai)
{
    int sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock == -1) {
        warn("socket() failed");
        return sock;
    }
    struct sockaddr* sa = ai->ai_addr;
    socklen_t salen = ai->ai_addrlen;
    if (connect(sock, sa, salen) != 0) {
        e = errno;
        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];
        getnameinfo(sa, salen, host, sizeof(host), serv, sizeof(serv), 0);
        /* Ignore error of getnameinfo(3). Failing of getnameinfo(3) is rare? */
        warnc(e, "cannot connect to %s:%s", host, serv);
        close(sock);
        return -1;
    }
    return sock;
}

static void
write_all(int wfd, const void* buf, size_t bufsize)
{
    size_t nbytes = 0;
    while (nbytes < bufsize) {
        const void* p = (void*)((uintptr_t)buf + nbytes);
        ssize_t n = write(wfd, p, bufsize - nbytes);
        if (n == -1) {
            err(1, "cannot write data");
        }
        nbytes += n;
    }
}

static void
write32(int wfd, uint32_t n)
{
    uint32_t datum = htonl(n);
    write_all(wfd, &datum, sizeof(datum));
}

static void
write_string(int wfd, const char* s)
{
    size_t len = strlen(s);
    if (UINT32_MAX < len) {
        die("the string is too long (%lu).", len);
    }
    write32(wfd, len);
    write_all(wfd, s, len);
}

static void
start_slave(int rfd, int argc, char* argv[])
{
    int wfd = dup(rfd);
    if (wfd == -1) {
        err(1, "dup() failed");
    }

    write32(wfd, argc);
    int i;
    for (i = 0; i < argc; i++) {
        write_string(wfd, argv[i]);
    }

    fsyscall_start_slave(rfd, wfd, argc, argv);
    /* NOTREACHED */
}

static void
log_starting(int argc, char* argv[])
{
    char buf[512];
    snprintf(buf, sizeof(buf), "%s", getprogname());

    int pos = strlen(buf);
    int i;
    for (i = 0; i < argc; i++) {
        const char* s = argv[i];
        size_t len = strlen(s);
        snprintf(buf + pos, sizeof(buf) - pos, " %s", s);
        pos += len + 1;
    }

    syslog(LOG_INFO, "started: %s", buf);
}

static void
nexec_main(int argc, char* argv[])
{
    if (argc < 2) {
        usage();
        exit(1);
    }
    log_starting(argc, argv);

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
    hints.ai_family = PF_UNSPEC;
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
        die("connecting to nexecd failed.");
    }

    syslog(LOG_INFO, "connected: host=%s, service=%s", hostname, servname);

    start_slave(sock, argc - 1, argv + 1);
}

int
main(int argc, char* argv[])
{
    struct option opts[] = {
        { "version", no_argument, NULL, 'v' },
        { NULL, 0, NULL, 0 } };
    int opt;
    while ((opt = getopt_long(argc, argv, "+v", opts, NULL)) != -1) {
        switch (opt) {
        case 'v':
            printf("%s %s\n", getprogname(), NEXEC_VERSION);
            return 0;
        default:
            break;
        }
    }

    openlog(getprogname(), LOG_PID, LOG_USER);
    nexec_main(argc - optind, argv + optind);
    closelog();

    return 0;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
