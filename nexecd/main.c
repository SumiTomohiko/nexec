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
#include <syslog.h>
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
read_all(int rfd, void* dest, size_t size)
{
    size_t nbytes = 0;
    while (nbytes < size) {
        void* p = (void*)((uintptr_t)dest + nbytes);
        ssize_t n = read(rfd, p, size - nbytes);
        if (n == -1) {
            err(1, "Cannot read data");
        }
        nbytes += n;
    }
}

static unsigned int
read32(int rfd)
{
    unsigned int n;
    read_all(rfd, &n, sizeof(n));
    return ntohl(n);
}

static void
start_master(int rfd)
{
    int wfd = dup(rfd);
    if (wfd == -1) {
        err(1, "dup() failed");
    }

    unsigned int argc = read32(rfd);
    char* argv[argc];
    unsigned int i;
    for (i = 0; i < argc; i++) {
        unsigned int len = read32(rfd);
        char* p = (char*)malloc(len + 1);
        assert(p != NULL);
        read_all(rfd, p, len);
        p[len] = '\0';
        argv[i] = p;
    }

    fsyscall_start_master(rfd, wfd, argc, argv);
    /* NOTREACHED */
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

    syslog(LOG_INFO, "Started.");

    int fd;
    struct sockaddr_storage storage;
    struct sockaddr* addr = (struct sockaddr*)&storage;
    socklen_t addrlen;
    while ((fd = accept(sock, addr, &addrlen)) != -1) {
        char host[NI_MAXHOST], serv[NI_MAXSERV];
        int ecode = getnameinfo(addr, addrlen, host, sizeof(host), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
        if (ecode != 0) {
            die("getnameinfo() failed: %s", gai_strerror(ecode));
        }
        syslog(LOG_INFO, "Accepted: host=%s, port=%s", host, serv);

        pid_t pid = fork();
        assert(pid != -1);
        int status;
        switch (pid) {
        case 0:
            start_master(fd);
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

    openlog(getprogname(), LOG_PID, LOG_USER);
    nexecd_main();
    closelog();

    return 0;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
