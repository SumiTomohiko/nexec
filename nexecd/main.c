#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <grp.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#include <nexec/config.h>
#include <nexec/nexecd.h>
#include <nexec/util.h>

static int
make_bound_socket(struct addrinfo* ai)
{
    int sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock == -1) {
        die("socket() failed: %s", strerror(errno));
    }
    int on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
        die("setsockopt() failed: %s", strerror(errno));
    }
    if (bind(sock, ai->ai_addr, ai->ai_addrlen) != 0) {
        die("bind() failed: %s", strerror(errno));
    }
    return sock;
}

static int
wait_client(int socks[], int nsock)
{
    fd_set fds;
    FD_ZERO(&fds);
    int i;
    for (i = 0; i < nsock; i++) {
        FD_SET(socks[i], &fds);
    }
    assert(0 < nsock);
    int max = socks[0];
    for (i = 1; i < nsock; i++) {
        int sock = socks[i];
        max = max < sock ? sock : max;
    }
    if (select(max + 1, &fds, NULL, NULL, NULL) == -1) {
        return -1;
    }
    int sock = -1;
    for (i = 0; (i < nsock) && (sock == -1); i++) {
        int fd = socks[i];
        sock = FD_ISSET(fd, &fds) ? fd : -1;
    }
    return sock;
}

static bool terminated = false;

static void
signal_handler(int sig)
{
    assert(sig == SIGTERM);
    terminated = true;
}

static void
listen_or_die(int sock)
{
    if (listen(sock, 0) != 0) {
        die("listen() failed for socket %d: %s", sock, strerror(errno));
    }
}

static void
read_config_or_die(struct config* config)
{
    const char* path = NEXEC_INSTALL_PREFIX "/etc/nexecd.conf";
    FILE* fpin = fopen(path, "r");
    if (fpin == NULL) {
        die("cannot open %s: %s", path, strerror(errno));
    }

    parser_initialize(config);
    extern FILE* yyin;
    yyin = fpin;
    extern int yyparse();
    yyparse();

    fclose(fpin);
}

static pid_t
fork_or_die()
{
    pid_t pid = fork();
    if (pid == -1) {
        die("fork failed: %s", strerror(errno));
    }
    return pid;
}

static void
start_child(struct config* config, int fd)
{
    pid_t pid = fork_or_die();
    if (pid != 0) {
        close(fd);
        waitpid(pid, NULL, 0);
        return;
    }

    pid_t pid2 = fork_or_die();
    if (pid2 != 0) {
        exit(0);
    }

    child_main(config, fd);
    /* UNREACHABLE */
    syslog(LOG_EMERG, "UNREACHABLE REACHED");
}

static void
daemon_or_die()
{
    if (daemon(0, 0) != 0) {
        die("daemon() failed: %s", strerror(errno));
    }
}

static void
set_user_group_or_die(struct config* config)
{
    errno = 0;
    const char* groupname = config->daemon.group;
    struct group* group = getgrnam(groupname);
    if (group == NULL) {
        die("cannot find group: %s: %s", groupname, strerror(errno));
    }
    gid_t gid = group->gr_gid;
    if (setgid(gid) != 0) {
        die("cannot set group: %s (%d): %s", groupname, gid, strerror(errno));
    }
    const char* username = config->daemon.user;
    struct passwd* user = getpwnam(username);
    if (user == NULL) {
        die("cannot find user: %s: %s", user, strerror(errno));
    }
    uid_t uid = user->pw_uid;
    if (setuid(uid) != 0) {
        die("cannot set user: %s (%d): %s", user, uid, strerror(errno));
    }
}

static void
nexecd_main()
{
    struct addrinfo hints;
    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo* ai;
    int ecode = getaddrinfo(NULL, "57005", &hints, &ai);
    if (ecode != 0) {
        die("getaddrinfo() failed: %s", gai_strerror(ecode));
    }
    int nsock = 0;
    struct addrinfo* res;
    for (res = ai; res != NULL; res = res->ai_next) {
        nsock++;
    }
    int socks[nsock];
    int i;
    for (res = ai, i = 0; res != NULL; res = res->ai_next, i++) {
        socks[i] = make_bound_socket(res);
    }
    freeaddrinfo(ai);
    for (i = 0; i < nsock; i++) {
        listen_or_die(socks[i]);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        die("signal() failed: %s", strerror(errno));
    }

    struct config config;
    config.daemon.user[0] = config.daemon.group[0] = '\0';
    read_config_or_die(&config);

    set_user_group_or_die(&config);
    daemon_or_die();
    syslog(LOG_INFO, "started.");

    int sock;
    while (!terminated && ((sock = wait_client(socks, nsock)) != -1)) {
        struct sockaddr_storage storage;
        struct sockaddr* addr = (struct sockaddr*)&storage;
        socklen_t addrlen = sizeof(storage);
        int fd = accept(sock, addr, &addrlen);
        if (fd == -1) {
            die("accept() failed: %s", strerror(errno));
        }

        char host[NI_MAXHOST], serv[NI_MAXSERV];
        int ecode = getnameinfo(addr, addrlen, host, sizeof(host), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
        if (ecode != 0) {
            die("getnameinfo() failed: %s", gai_strerror(ecode));
        }
        syslog(LOG_INFO, "accepted: host=%s, port=%s", host, serv);

        start_child(&config, fd);
    }

    for (i = 0; i < nsock; i++) {
        close(socks[i]);
    }

    syslog(LOG_INFO, "terminated.");
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
    syslog(LOG_INFO, "initializing.");
    memory_initialize();
    nexecd_main();
    memory_dispose();
    closelog();

    return 0;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
