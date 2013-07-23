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
#include <nexec/util.h>

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
write_ok(int fd)
{
    writeln(fd, "OK");
}

static void
write_ng(int fd, const char* msg)
{
    char buf[1024];
    snprintf(buf, sizeof(buf), "NG %s", msg);
    syslog(LOG_ERR, "%s", buf);
    writeln(fd, buf);
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
setblock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
        die("fcntl() failed to be blocking: %s", strerror(errno));
    }
}

static char*
find_mapping(struct config* config, const char* name)
{
    struct mapping* mappings = config->mappings;
    while ((mappings != NULL) && (strcmp(name, mappings->name) != 0)) {
        mappings = mappings->next;
    }
    return mappings != NULL ? mappings->path : NULL;
}

static void
do_exec(struct config* config, int fd, struct tokenizer* tokenizer)
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

    char* exe = find_mapping(config, args[0]);
    if (exe == NULL) {
        write_ng(fd, "command not found");
        return;
    }
    args[0] = exe;  /* This is not beautiful. */

    write_ok(fd);

    setblock(fd);
    start_master(fd, nargs, args);
}

static void
handle_request(struct config* config, int fd)
{
    char line[4096];
    read_line(fd, line, sizeof(line));
    syslog(LOG_INFO, "request: %s", line);
    die_if_invalid_line(line);

    struct tokenizer tokenizer;
    tokenizer.p = line;
    char* token = get_next_token(&tokenizer);
    if (strcmp(token, "EXEC") == 0) {
        do_exec(config, fd, &tokenizer);
        return;
    }
    write_ng(fd, "unknown command");
}

static void
setnonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        die("fcntl() failed to be non-blocking: %s", strerror(errno));
    }
}

void
child_main(struct config* config, int fd)
{
    setnonblock(fd);

    while (1) {
        handle_request(config, fd);
    }

    /* NOTREACHED */
    exit(1);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
