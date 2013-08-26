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
#include <nexec/util.h>

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
        int e = errno;
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
write_exec(int fd, int argc, char* argv[])
{
    const char* cmd = "EXEC";
    size_t cmd_size = strlen(cmd);

    size_t args_size = 0;
    int i;
    for (i = 0; i < argc; i++) {
        args_size += 1 + strlen(argv[i]);
    }

    char buf[cmd_size + args_size + 1];
    memcpy(buf, cmd, cmd_size);
    char* p = buf + cmd_size;
    for (i = 0; i < argc; i++) {
        sprintf(p, " %s", argv[i]);
        p += strlen(p);
    }
    *p = '\0';

    writeln(fd, buf);
}

static void
die_if_ng(int fd)
{
    char buf[4096];
    read_line(fd, buf, sizeof(buf));
    syslog(LOG_DEBUG, "read: %s", buf);
    if (strcmp(buf, "OK") == 0) {
        return;
    }
    die("request failed: %s", buf);
}

static void
start_slave(int rfd, int argc, char* argv[])
{
    int wfd = dup(rfd);
    if (wfd == -1) {
        err(1, "dup() failed");
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
do_exec(int fd, int argc, char* argv[])
{
    write_exec(fd, argc, argv);
    die_if_ng(fd);
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
    set_tcp_nodelay_or_die(sock);

    syslog(LOG_INFO, "connected: host=%s, service=%s", hostname, servname);

    int nargs = argc - 1;
    char** args = argv + 1;
    do_exec(sock, nargs, args);
    start_slave(sock, nargs, args);
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
