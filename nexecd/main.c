#include <sys/param.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
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
#include <syslog.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <fsyscall/private/die.h>
#include <nexec/config.h>
#include <nexec/nexecd.h>
#include <nexec/util.h>

#define SETTINGS_DIR    NEXEC_INSTALL_PREFIX "/etc/nexecd"

static int
make_bound_socket(struct addrinfo *ai)
{
	int on, sock;

	sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (sock == -1)
		die(1, "socket() failed");
	on = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0)
		die(1, "setsockopt() failed");
	if (bind(sock, ai->ai_addr, ai->ai_addrlen) != 0)
		die(1, "bind() failed");

	return (sock);
}

static int
wait_client(int socks[], int nsock)
{
	fd_set fds;
	int fd, i, max, sock;

	FD_ZERO(&fds);
	for (i = 0; i < nsock; i++)
		FD_SET(socks[i], &fds);
	assert(0 < nsock);
	max = socks[0];
	for (i = 1; i < nsock; i++) {
		sock = socks[i];
		max = max < sock ? sock : max;
	}
	if (select(max + 1, &fds, NULL, NULL, NULL) == -1)
		return (-1);
	sock = -1;
	for (i = 0; (i < nsock) && (sock == -1); i++) {
		fd = socks[i];
		sock = FD_ISSET(fd, &fds) ? fd : -1;
	}

	return (sock);
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

	if (listen(sock, 0) != 0)
		die(1, "listen() failed for socket %d", sock);
}

static void
read_config_or_die(struct config *config)
{
	FILE *fpin;
	const char *path;

	path = SETTINGS_DIR "/main.conf";
	fpin = fopen(path, "r");
	if (fpin == NULL)
		die(1, "cannot open %s", path);

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
	pid_t pid;

	pid = fork();
	if (pid == -1)
		die(1, "fork failed");

	return (pid);
}

static void
start_child(struct config *config, int fd)
{
	pid_t pid, pid2;

	pid = fork_or_die();
	if (pid != 0) {
		close(fd);
		waitpid(pid, NULL, 0);
		return;
	}

	pid2 = fork_or_die();
	if (pid2 != 0)
		exit(0);

	child_main(config, fd);

	exit(0);
}

static void
daemon_or_die()
{

	if (daemon(0, 0) != 0)
		die(1, "daemon() failed");
}

static void
set_user_group_or_die(struct config *config)
{
	struct passwd *user;
	struct group *group;
	uid_t uid;
	gid_t gid;
	const char *groupname, *username;

	errno = 0;
	groupname = config->daemon.group;
	group = getgrnam(groupname);
	if (group == NULL)
		die(1, "cannot find group: %s: %s", groupname);
	gid = group->gr_gid;
	if (setgid(gid) != 0)
		die(1, "cannot set group: %s (%d)", groupname, gid);
	username = config->daemon.user;
	user = getpwnam(username);
	if (user == NULL)
		die(1, "cannot find user: %s", user);
	uid = user->pw_uid;
	if (setuid(uid) != 0)
		die(1, "cannot set user: %s (%d)", user, uid);
}

static void
nexecd_main(SSL_CTX *ctx)
{
	struct config config;
	struct addrinfo *ai, hints, *res;
	struct sockaddr_storage storage;
	struct sockaddr *addr;
	socklen_t addrlen;
	int ecode, fd, i, nsock, sock, *socks;
	char host[NI_MAXHOST], serv[NI_MAXSERV];

	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	ecode = getaddrinfo(NULL, "57005", &hints, &ai);
	if (ecode != 0)
		die(1, "getaddrinfo() failed: %s", gai_strerror(ecode));
	nsock = 0;
	for (res = ai; res != NULL; res = res->ai_next)
		nsock++;
	socks = (int *)alloca(sizeof(int) * nsock);
	for (res = ai, i = 0; res != NULL; res = res->ai_next, i++)
		socks[i] = make_bound_socket(res);
	freeaddrinfo(ai);
	for (i = 0; i < nsock; i++)
		listen_or_die(socks[i]);
	if (signal(SIGTERM, signal_handler) == SIG_ERR)
		die(1, "signal() failed");

	config.ssl.ctx = ctx;
	config.daemon.user[0] = config.daemon.group[0] = '\0';
	read_config_or_die(&config);

	set_user_group_or_die(&config);
	daemon_or_die();
	syslog(LOG_INFO, "started.");

	while (!terminated && ((sock = wait_client(socks, nsock)) != -1)) {
		addr = (struct sockaddr *)&storage;
		addrlen = sizeof(storage);
		fd = accept(sock, addr, &addrlen);
		if (fd == -1)
			die(1, "accept() failed");
		set_tcp_nodelay_or_die(fd);

		ecode = getnameinfo(addr, addrlen, host, sizeof(host), serv,
				    sizeof(serv),
				    NI_NUMERICHOST | NI_NUMERICSERV);
		if (ecode != 0)
			die(1, "getnameinfo() failed: %s", gai_strerror(ecode));
		syslog(LOG_INFO, "accepted: host=%s, port=%s", host, serv);

		start_child(&config, fd);
	}

	for (i = 0; i < nsock; i++)
		close(socks[i]);

	syslog(LOG_INFO, "terminated.");
}

static SSL_CTX *
create_ctx()
{
	SSL_CTX *ctx;
	int ret;

	SSL_library_init();
	SSL_load_error_strings();

	ctx = SSL_CTX_new(SSLv23_server_method());
	if (ctx == NULL)
		die(1, "cannot SSL_CTX_new(3)");
	ret = SSL_CTX_use_certificate_file(ctx, SETTINGS_DIR "/cert.pem",
					   SSL_FILETYPE_PEM);
	if (ret != 1) {
		ERR_print_errors_fp(stderr);
		die(1, "cannot SSL_CTX_use_certificate_file(3)");
	}
	ret = SSL_CTX_use_PrivateKey_file(ctx, SETTINGS_DIR "/private.pem",
					  SSL_FILETYPE_PEM);
	if (ret != 1) {
		ERR_print_errors_fp(stderr);
		die(1, "cannot SSL_CTX_use_PrivateKey_file(3)");
	}
	if (!SSL_CTX_check_private_key(ctx))
		die(1, "wrong keys");

	return (ctx);
}

int
main(int argc, char* argv[])
{
	SSL_CTX *ctx;
	struct option opts[] = {
		{ "version", no_argument, NULL, 'v' },
		{ NULL, 0, NULL, 0 } };
	int opt;

	while ((opt = getopt_long(argc, argv, "+v", opts, NULL)) != -1)
		switch (opt) {
		case 'v':
			printf("%s %s\n", getprogname(), NEXEC_VERSION);
			return 0;
		default:
			break;
		}

	openlog(getprogname(), LOG_PID, LOG_USER);
	syslog(LOG_INFO, "initializing.");
	memory_initialize();

	ctx = create_ctx();
	nexecd_main(ctx);
	SSL_CTX_free(ctx);

	memory_dispose();
	closelog();

	return (0);
}
