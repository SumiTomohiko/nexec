%{
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <nexec/nexecd.h>

#include "parser.h"

static int
yywrap()
{

	return (1);
}

static char string[256];
%}
%start COMMENT STRING
%%
<INITIAL>":"        return T_COLON;
<INITIAL>"daemon"   return T_DAEMON;
<INITIAL>"user"     return T_USER_;     /* Read parser.y about the last "_" */
<INITIAL>"group"    return T_GROUP;
<INITIAL>"end"      return T_END;
<INITIAL>"mapping"  return T_MAPPING;
<INITIAL>"\n"       return T_NEWLINE;
<INITIAL>"\""       {
	string[0] = '\0';
	BEGIN STRING;
}
<INITIAL>"#"        {
	BEGIN COMMENT;
}
<INITIAL>[ \t]      ;
<INITIAL>.          {
	abort();
}

<STRING>"\""        {
	size_t size;
	char *ptr;

	size = strlen(string) + 1;
	ptr = memory_allocate(size);
	memcpy(ptr, string, size);
	yylval.string = ptr;
	BEGIN INITIAL;
	return (T_STRING);
}
<STRING>.           {
	assert(strlen(string) < sizeof(string));
	strcat(string, yytext);
}

<COMMENT>"\n"       {
	BEGIN INITIAL;
}
<COMMENT>.
%%
