#include <sys/types.h>
#include <sys/socket.h>
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
#include <syslog.h>
#include <unistd.h>

#include <fsyscall/private/die.h>
#include <fsyscall/start_slave.h>
#include <openssl/ssl.h>

#include <nexec/config.h>
#include <nexec/util.h>

static void
usage()
{

	printf("usage: %s hostname[:port] command...\n", getprogname());
}

static int
make_connected_socket(struct addrinfo *ai)
{
	struct sockaddr *sa;
	socklen_t salen;
	int e, sock;
	char host[NI_MAXHOST], serv[NI_MAXSERV];

	sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (sock == -1) {
		warn("socket() failed");
		return (sock);
	}
	sa = ai->ai_addr;
	salen = ai->ai_addrlen;
	if (connect(sock, sa, salen) != 0) {
		e = errno;
		getnameinfo(sa, salen, host, sizeof(host), serv, sizeof(serv),
			    0);
		/*
		 * Ignore error of getnameinfo(3). Failing of getnameinfo(3) is
		 * rare?
		 */
		warnc(e, "cannot connect to %s:%s", host, serv);
		close(sock);
		return (-1);
	}
	return (sock);
}

static void
write_exec(SSL *ssl, int argc, char *argv[])
{
	size_t args_size, cmd_size;
	int i;
	const char *cmd;
	char *buf, *p;

	cmd = "EXEC";
	cmd_size = strlen(cmd);

	args_size = 0;
	for (i = 0; i < argc; i++)
		args_size += 1 + strlen(argv[i]);
	buf = (char *)alloca(cmd_size + args_size + 1);
	memcpy(buf, cmd, cmd_size);
	p = buf + cmd_size;
	for (i = 0; i < argc; i++) {
		sprintf(p, " %s", argv[i]);
		p += strlen(p);
	}
	*p = '\0';

	writeln(ssl, buf);
}

static void
die_if_ng(SSL *ssl)
{
	char buf[4096];

	read_line(ssl, buf, sizeof(buf));
	syslog(LOG_DEBUG, "read: %s", buf);
	if (strcmp(buf, "OK") == 0)
		return;
	die(1, "request failed: %s", buf);
}

#if 0
static void
start_slave(SSL *ssl, int argc, char *argv[])
{
	/*
	 * THIS FUNCTION IS NOT IMPLEMENTED.
	 */
	fsyscall_run_slave_ssl(ssl, argc, argv);
}
#endif

static void
log_starting(int argc, char *argv[])
{
	size_t len;
	int i, pos;
	const char *s;
	char buf[512];

	snprintf(buf, sizeof(buf), "%s", getprogname());

	pos = strlen(buf);
	for (i = 0; i < argc; i++) {
		s = argv[i];
		len = strlen(s);
		snprintf(buf + pos, sizeof(buf) - pos, " %s", s);
		pos += len + 1;
	}

	syslog(LOG_INFO, "started: %s", buf);
}

static void
do_exec(SSL *ssl, int argc, char *argv[])
{

	write_exec(ssl, argc, argv);
	die_if_ng(ssl);
}

static void
do_set_env(SSL *ssl, struct env *penv)
{
	const char *cmd, *name, *value;
	char *buf;

	cmd = "SET_ENV";
	name = penv->name;
	value = penv->value;
	buf = (char *)alloca(strlen(cmd) + strlen(name) + strlen(value) + 3);
	sprintf(buf, "%s %s %s", cmd, name, value);
	writeln(ssl, buf);

	die_if_ng(ssl);
}

static void
send_env(SSL *ssl, struct env *penv)
{
	struct env *p;

	for (p = penv; p != NULL; p = p->next)
		do_set_env(ssl, p);
}

static int
nexec_main(int argc, char *argv[], struct env *penv)
{
	SSL_CTX *ctx;
	SSL *ssl;
	struct addrinfo *ai, hints, *res;
	int ecode, nargs, sock;
	const char *hostname, *s, *servname;
	char **args, *buf, *p;

	if (argc < 2) {
		usage();
		return (1);
	}
	log_starting(argc, argv);

	s = argv[0];
	buf = (char *)alloca(strlen(s) + 1);
	strcpy(buf, s);
	p = strrchr(buf, ':');
	if (p != NULL)
		*p = '\0';
	hostname = buf;
	servname = p != NULL ? &p[1] : NEXEC_DEFAULT_PORT;
	bzero(&hints, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	ecode = getaddrinfo(hostname, servname, &hints, &ai);
	if (ecode != 0)
		die(1, "getaddrinfo() failed: %s", gai_strerror(ecode));
	sock = -1;
	for (res = ai; (res != NULL) && (sock == -1); res = res->ai_next)
		sock = make_connected_socket(res);
	freeaddrinfo(ai);
	if (sock == -1)
		die(1, "connecting to nexecd failed.");
	set_tcp_nodelay_or_die(sock);

	syslog(LOG_INFO, "connected: host=%s, service=%s", hostname, servname);

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
		die(1, "cannot SSL_CTX_new(3)");
	ssl = SSL_new(ctx);
	if (ssl == NULL)
		die(1, "cannot SSL_new(3)");
	if (SSL_set_fd(ssl, sock) != 1)
		die(1, "cannot SSL_set_fd(3)");
	if (SSL_connect(ssl) != 1)
		die(1, "cannot SSL_connect(3)");

	send_env(ssl, penv);

	nargs = argc - 1;
	args = argv + 1;
	do_exec(ssl, nargs, args);
#if 0
	start_slave(ssl, nargs, args);
#endif
	exit(1);
	/* NOTREACHED */
	return (1);
}

static struct env *
create_env(const char *pair)
{
	struct env *penv;
	char *buf, *name, *p, *value;

	buf = (char *)alloca(strlen(pair) + 1);
	strcpy(buf, pair);
	p = strchr(buf, '=');
	assert(p != NULL);
	*p = '\0';
	p++;

	name = (char *)malloc(strlen(buf) + 1);
	assert(name != NULL);
	strcpy(name, buf);
	value = (char *)malloc(strlen(p) + 1);
	assert(value != NULL);
	strcpy(value, p);

	penv = alloc_env_or_die();
	penv->next = NULL;
	penv->name = name;
	penv->value = value;

	return (penv);
}

int
main(int argc, char *argv[])
{
	struct env *p, *penv;
	struct option opts[] = {
		{ "env", required_argument, NULL, 'e' },
		{ "version", no_argument, NULL, 'v' },
		{ NULL, 0, NULL, 0 } };
	int opt, status;

	penv =  NULL;
	while ((opt = getopt_long(argc, argv, "+v", opts, NULL)) != -1)
		switch (opt) {
		case 'e':
			p = create_env(optarg);
			p->next = penv;
			penv = p;
			break;
		case 'v':
			printf("%s %s\n", getprogname(), NEXEC_VERSION);
			return (0);
		default:
			break;
		}

	openlog(getprogname(), LOG_PID, LOG_USER);
	status = nexec_main(argc - optind, argv + optind, penv);
	closelog();

	return (status);
}
