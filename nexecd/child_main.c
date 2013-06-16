#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <fsyscall/start_master.h>

#include <nexec/nexecd.h>

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

    va_start(ap, fmt);
    vsyslog(LOG_ERR, fmt, ap);
    va_end(ap);

    exit(1);
}

struct tokenizer {
    char* p;
};

static void
skip_whitespace(struct tokenizer* tokenizer)
{
    char *p;
    for (p = tokenizer->p; (*p != '\0') && (*p == ' '); p++) {
    }
    tokenizer->p = p;
}

static char*
get_next_token(struct tokenizer* tokenizer)
{
    skip_whitespace(tokenizer);

    char *begin = tokenizer->p;
    char *p;
    for (p = begin; (*p != '\0') && (*p != ' '); p++) {
    }
    tokenizer->p = p + (*p == '\0' ? 0 : 1);
    *p = '\0';
    return begin;
}

static void
die_if_invalid_byte(char c)
{
    if (isprint(c)) {
        return;
    }
    die("invalid byte found: 0x%02x", 0xff & c);
}

static void
die_if_invalid_line(char *s)
{
    char *pend = s + strlen(s);
    char *p;
    for (p = s; p < pend; p++) {
        die_if_invalid_byte(*p);
    }
}

static void
die_if_timeout(time_t t0)
{
    if (time(NULL) - t0 < 60) {
        return;
    }
    die("timeout");
}

static char
read_char(int fd)
{
    time_t t0 = time(NULL);

    ssize_t n;
    char c;
    while (((n = read(fd, &c, sizeof(c))) == -1) && (errno == EAGAIN)) {
        die_if_timeout(t0);

        struct timespec rqtp;
        rqtp.tv_sec = 0;
        rqtp.tv_nsec = 1000000;
        nanosleep(&rqtp, NULL);
    }
    if (n == -1) {
        die("cannot read a next char: %s", strerror(errno));
    }

    return c;
}

static void
read_line(int fd, char* buf, size_t bufsize)
{
    char* pend = buf + bufsize;
    char* p = buf;
    char c;
    while ((p < pend) && (c = read_char(fd)) != '\r') {
        *p = c;
        p++;
    }
    if (p == pend) {
        p[-1] = '\0';
        die("too long request: %s", buf);
    }

    if (read_char(fd) != '\n') {
        die("invalid line terminator.");
    }

    *p = '\0';
}

static void
write_all(int fd, char* buf, size_t bufsize)
{
    char* pend = buf + bufsize;
    size_t rest = bufsize;
    while (0 < rest) {
        ssize_t n = write(fd, pend - rest, rest);
        if ((n == -1) && (errno != EAGAIN)) {
            die("cannot write: %s", strerror(errno));
        }
        rest -= (n == -1 ? 0 : n);
    }
}

static void
write_ok(int fd)
{
    char* buf = "OK\r\n";
    write_all(fd, buf, strlen(buf));
}

static void
write_ng(int fd, const char* msg)
{
    char buf[1024];
    snprintf(buf, sizeof(buf), "NG %s", msg);
    syslog(LOG_ERR, "%s", buf);

    char buf2[1024];
    snprintf(buf2, sizeof(buf2), "%s\r\n", buf);
    write_all(fd, buf2, strlen(buf2));
}

static void
start_master(int rfd, int argc, char* argv[])
{
    int wfd = dup(rfd);
    if (wfd == -1) {
        die("dup() failed: %s", strerror(errno));
    }

    fsyscall_start_master(rfd, wfd, argc, argv);
    /* NOTREACHED */
}

static void
do_exec(int fd, struct tokenizer* tokenizer)
{
#define MAX_NARGS 64
    char* args[MAX_NARGS];
    int nargs = 0;
    char* p = get_next_token(tokenizer);
    while ((0 < strlen(p)) && (nargs < MAX_NARGS)) {
        args[nargs] = p;
        nargs++;

        p = get_next_token(tokenizer);
    }
    if (nargs == MAX_NARGS) {
        write_ng(fd, "too many arguments");
        return;
    }
#undef MAX_NARGS
    if (nargs == 0) {
        write_ng(fd, "no arguments given");
        return;
    }

    write_ok(fd);
    start_master(fd, nargs, args);
}

static void
handle_request(int fd)
{
    char line[4096];
    read_line(fd, line, sizeof(line));
    syslog(LOG_INFO, "request: %s", line);
    die_if_invalid_line(line);

    struct tokenizer tokenizer;
    tokenizer.p = line;
    char* token = get_next_token(&tokenizer);
    if (strcmp(token, "EXEC") == 0) {
        do_exec(fd, &tokenizer);
        return;
    }
    write_ng(fd, "unknown command");
}

void
child_main(int fd)
{
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        die("fcntl() failed for F_SETFL and O_NONBCLOCK: %s", strerror(errno));
    }

    while (1) {
        handle_request(fd);
    }

    /* NOTREACHED */
    exit(1);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
