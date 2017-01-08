#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <fsyscall/private/die.h>
#include <fsyscall/run_master.h>
#include <openssl/ssl.h>

#include <nexec/nexecd.h>
#include <nexec/util.h>

struct child {
    struct config* config;
    struct env* env;
    SSL* ssl;
};

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
    die(1, "invalid byte found: 0x%02x", 0xff & c);
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
write_ok(SSL* ssl)
{
    writeln(ssl, "OK");
}

static void
write_ng(SSL* ssl, const char* msg)
{
    char buf[1024];
    snprintf(buf, sizeof(buf), "NG %s", msg);
    syslog(LOG_ERR, "%s", buf);
    writeln(ssl, buf);
}

static void
start_master(SSL* ssl, int argc, char* argv[], char* const envp[])
{
    fsyscall_run_master_ssl(ssl, argc, argv, envp);
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

static char*
copy_next_token(struct tokenizer* tokenizer)
{
    const char* s = get_next_token(tokenizer);
    char* t = (char*)malloc(strlen(s) + 1);
    assert(t != NULL);
    strcpy(t, s);
    return t;
}

static void
do_set_env(struct child* child, struct tokenizer* tokenizer)
{
    struct env* penv = alloc_env_or_die();
    penv->next = child->env;
    penv->name = copy_next_token(tokenizer);
    penv->value = copy_next_token(tokenizer);
    child->env = penv;

    write_ok(child->ssl);
}

static int
count_env(struct env* env)
{
    int n = 0;
    struct env* penv;
    for (penv = env; penv != NULL; penv = penv->next) {
        n++;
    }
    return n;
}

static char*
format_env(const char* name, const char* value)
{
    char* s = (char*)malloc(strlen(name) + strlen(value) + 2);
    assert(s != NULL);
    sprintf(s, "%s=%s", name, value);

    return s;
}

static void
do_exec(struct child* child, struct tokenizer* tokenizer)
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
        write_ng(child->ssl, "too many arguments");
        return;
    }
#undef MAX_NARGS
    if (nargs == 0) {
        write_ng(child->ssl, "no arguments given");
        return;
    }

    char* exe = find_mapping(child->config, args[0]);
    if (exe == NULL) {
        write_ng(child->ssl, "command not found");
        return;
    }
    args[0] = exe;  /* This is not beautiful. */

    int nenv = count_env(child->env);
    char** envp = (char**)malloc(sizeof(char*) * (nenv + 1 /* PATH */ + 1 /* NULL */));
    assert(envp != NULL);
    struct env* penv;
    int i;
    for (penv = child->env, i = 0; penv != NULL; penv = penv->next, i++) {
        envp[i] = format_env(penv->name, penv->value);
    }
    envp[i] = format_env("PATH", "/usr/local/bin:/usr/bin:/bin");
    i++;
    envp[i] = NULL;

    write_ok(child->ssl);
    start_master(child->ssl, nargs, args, envp);
}

static bool
handle_request(struct child* child)
{
    char line[4096];
    read_line(child->ssl, line, sizeof(line));
    syslog(LOG_INFO, "request: %s", line);
    die_if_invalid_line(line);

    struct tokenizer tokenizer;
    tokenizer.p = line;
    char* token = get_next_token(&tokenizer);
    if (strcmp(token, "EXEC") == 0) {
        do_exec(child, &tokenizer);
        return true;
    }
    if (strcmp(token, "SET_ENV") == 0) {
        do_set_env(child, &tokenizer);
        return false;
    }
    write_ng(child->ssl, "unknown command");
    return false;
}

static void
setnonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        die(1, "fcntl() failed to be non-blocking");
    }
}

void
child_main(struct config* config, int fd)
{
    setnonblock(fd);

    SSL* ssl = SSL_new(config->ssl.ctx);
    if (ssl == NULL) {
        die(1, "cannot SSL_new(3)");
    }
    if (SSL_set_fd(ssl, fd) != 1) {
        die(1, "cannot SSL_set_fd(3)");
    }
    int ret;
    while ((ret = SSL_accept(ssl)) != 1) {
        sslutil_handle_error(ssl, ret, "SSL_accept(3)");
    }

    struct child child;
    child.config = config;
    child.env = NULL;
    child.ssl = ssl;

    while (!handle_request(&child)) {
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(fd);

    exit(0);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
