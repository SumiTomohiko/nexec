#include <stdbool.h>
#include <stdio.h>

#include <nexec/nexecd.h>
#include "cmdlexer.h"

struct buf {
	char	*from;
	char	*to;
	char	*cur;
};

struct cmdlexer_scanner {
	enum YYCONDTYPE	cond;
	bool		eof;
	struct buf	buf;
	const char	*cur;
};

struct cmdlexer_scanner *
cmdlexer_create_scanner()
{

	return (memory_allocate(sizeof(struct cmdlexer_scanner)));
}

static void
buf_clear(struct buf *buf)
{

	buf->cur = buf->from;
	*buf->cur = '\0';
}

static int
buf_add_char(struct buf *buf, char c)
{

	if (buf->cur + 1 == buf->to)
		return (-1);

	*buf->cur = c;
	buf->cur++;
	*buf->cur = '\0';

	return (0);
}

void
cmdlexer_scan(struct cmdlexer_scanner *scanner, const char *line)
{
	size_t bufsize;

	bufsize = 256;

	scanner->cur = line;
	scanner->buf.from = (char *)memory_allocate(bufsize);
	scanner->buf.to = &scanner->buf.from[bufsize];
	scanner->eof = false;
}

#define	YYSETCONDITION(c)	scanner->cond = (c)
#undef	YYDEBUG
#define	YYDEBUG(state, current)	do {				\
	/*printf("state=%d, current=%c\n", (state), (current));*/	\
} while (0)

union token {
	enum command command;
	char *string;
};

#define	RETURN_COMMAND(cmd)	do {	\
	token->command = (cmd);		\
	return (SCANNER_SUCCESS);	\
} while (0)
#define	BUF_ADD_CHAR(c)					\
	if (buf_add_char(&scanner->buf, (c)) == -1)	\
		return (SCANNER_ERROR);			\
	continue
#define	BUF_ADD_LAST_CHAR	BUF_ADD_CHAR(scanner->cur[-1])
#define	END_SCAN		do {	\
	scanner->eof = true;		\
	return (SCANNER_END);		\
} while (0)

static enum scanner_status
cmdlexer_next(struct cmdlexer_scanner *scanner, union token *token)
{
	const char *ctxmarker, *marker;
	char *p;

	if (scanner->eof)
		return (SCANNER_END);

	ctxmarker = marker = NULL;
	for (;;) {
	/*!re2c

	re2c:yyfill:enable = 0;
	re2c:define:YYCTYPE = char;
	re2c:define:YYCURSOR = scanner->cur;
	re2c:define:YYGETCONDITION = scanner->cond;
	re2c:define:YYGETCONDITION:naked = 1;
	re2c:define:YYMARKER = marker;
	re2c:define:YYCTXMARKER = ctxmarker;

	b = [ \x00];
	ws = " ";
	end = "\x00";

	<command>ws+		{ continue; }
	<command>end		{ END_SCAN; }
	<command>"LOGIN"/b	{ RETURN_COMMAND(CMD_LOGIN); }
	<command>"SET_ENV"/b	{ RETURN_COMMAND(CMD_SET_ENV); }
	<command>"EXEC"/b	{ RETURN_COMMAND(CMD_EXEC); }

	<string>ws+		{ continue; }
	<string>end		{ END_SCAN; }
	<string>"\""		:=> quoted_string
	<string>.		=> plain_string { BUF_ADD_LAST_CHAR; }

	<quoted_string>"\\\""	{ BUF_ADD_CHAR('\"'); }
	<quoted_string>"\\\\"	{ BUF_ADD_CHAR('\\'); }
	<quoted_string>"\""	{ goto out; }
	<quoted_string>.	{ BUF_ADD_LAST_CHAR; }

	<plain_string>ws	{ goto out; }
	<plain_string>end	{
		scanner->eof = true;
		goto out;
	}
	<plain_string>.		{ BUF_ADD_LAST_CHAR; }

	 */
	}

out:
	p = (char *)memory_allocate(strlen(scanner->buf.from) + 1);
	strcpy(p, scanner->buf.from);
	token->string = p;

	return (SCANNER_SUCCESS);
}

enum scanner_status
cmdlexer_next_string(struct cmdlexer_scanner *scanner, char **out)
{
	union token token;
	int status;

	scanner->cond = yycstring;
	buf_clear(&scanner->buf);
	status = cmdlexer_next(scanner, &token);
	*out = token.string;

	return (status);
}

enum scanner_status
cmdlexer_next_command(struct cmdlexer_scanner *scanner, enum command *out)
{
	union token token;
	int status;

	scanner->cond = yyccommand;
	status = cmdlexer_next(scanner, &token);
	*out = token.command;

	return (status);
}

#if defined(LEXERTEST)
static void
t(int lineno, const char *in, int (*f)(struct cmdlexer_scanner *, void *),
  void *bonus)
{
	struct cmdlexer_scanner *scanner;
	int status;
	const char *result;
	char buf[8192];

	scanner = cmdlexer_create_scanner();
	cmdlexer_scan(scanner, in);

	status = f(scanner, bonus);
	if (status == 0)
		result = "OK";
	else {
		snprintf(buf, sizeof(buf), "NG (actual: %d)", status);
		result = buf;
	}
	printf("%d: %s: %s\n", lineno, in, result);
}

static int
test2(struct cmdlexer_scanner *scanner, void *bonus)
{
	const char *expected;
	char *actual;

	if (cmdlexer_next_string(scanner, &actual) != SCANNER_SUCCESS)
		return (1);
	expected = (const char *)bonus;
	if (strcmp(expected, actual) != 0)
		return (2);

	return (0);
}

static int
test3(struct cmdlexer_scanner *scanner, void *bonus)
{
	const char *expected;
	char *_, *actual;

	if (cmdlexer_next_string(scanner, &_) != SCANNER_SUCCESS)
		return (1);
	if (cmdlexer_next_string(scanner, &actual) != SCANNER_SUCCESS)
		return (2);
	expected = (const char *)bonus;
	if (strcmp(expected, actual) != 0)
		return (3);

	return (0);
}

static int
test4(struct cmdlexer_scanner *scanner, void *bonus)
{
	enum command _;
	const char *expected;
	char *actual;

	if (cmdlexer_next_command(scanner, &_) != SCANNER_SUCCESS)
		return (1);
	if (cmdlexer_next_string(scanner, &actual) != SCANNER_SUCCESS)
		return (2);
	expected = (const char *)bonus;
	if (strcmp(expected, actual) != 0)
		return (3);

	return (0);
}

static int
test5(struct cmdlexer_scanner *scanner, void *bonus)
{
	enum command _;
	const char *expected;
	char *__, *actual;

	if (cmdlexer_next_command(scanner, &_) != SCANNER_SUCCESS)
		return (1);
	if (cmdlexer_next_string(scanner, &__) != SCANNER_SUCCESS)
		return (2);
	if (cmdlexer_next_string(scanner, &actual) != SCANNER_SUCCESS)
		return (3);
	expected = (const char *)bonus;
	if (strcmp(expected, actual) != 0)
		return (4);

	return (0);
}

static int
test6(struct cmdlexer_scanner *scanner, void *bonus)
{
	char *_;

	if (cmdlexer_next_string(scanner, &_) != SCANNER_SUCCESS)
		return (1);
	if (cmdlexer_next_string(scanner, &_) != SCANNER_END)
		return (2);

	return (0);
}

static int
test7(struct cmdlexer_scanner *scanner, void *bonus)
{
	char *_;

	if (cmdlexer_next_string(scanner, &_) != SCANNER_END)
		return (1);
	if (cmdlexer_next_string(scanner, &_) != SCANNER_END)
		return (2);

	return (0);
}

static int
test8(struct cmdlexer_scanner *scanner, void *bonus)
{
	char *_;

	if (cmdlexer_next_string(scanner, &_) != SCANNER_ERROR)
		return (1);

	return (0);
}

static int
test(struct cmdlexer_scanner *scanner, void *bonus)
{
	enum command actual, expected;

	if (cmdlexer_next_command(scanner, &actual) != SCANNER_SUCCESS)
		return (1);
	expected = (enum command)bonus;
	if (actual != expected)
		return (2);

	return (0);
}

#define	T(in, f, bonus)	t(__LINE__, (in), (f), (bonus))

int
main()
{

	memory_initialize();

	T("LOGIN", test, (void *)CMD_LOGIN);
	T(" LOGIN", test, (void *)CMD_LOGIN);
	T("LOGIN foo", test, (void *)CMD_LOGIN);
	T("anonymous", test2, "anonymous");
	T(" anonymous", test2, "anonymous");
	T("anonymous foo", test2, "anonymous");
	T("\"anonymous\"", test2, "anonymous");
	T(" \"anonymous\"", test2, "anonymous");
	T("\"anonymous\" foo", test2, "anonymous");
	T("foo\"bar\"baz", test2, "foo\"bar\"baz");
	T("foo\\bar\\baz", test2, "foo\\bar\\baz");
	T("foo\\\"bar\\\"baz", test2, "foo\\\"bar\\\"baz");
	T("\"foo\\\"bar\\\"baz\"", test2, "foo\"bar\"baz");
	T("\"foo\\\\bar\\\\baz\"", test2, "foo\\bar\\baz");
	T("foo bar", test3, "bar");
	T("LOGIN foo", test4, "foo");
	T("LOGIN \"username\" \"password\"", test4, "username");
	T("LOGIN \"username\" \"password\"", test5, "password");
	T("foo", test6, NULL);
	T("", test7, NULL);
	T("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", test8, NULL);

	return (0);
}
#endif

/*
 * vim: filetype=c
 */
