%{
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <nexec/nexecd.h>

static struct config* parser_config;

void
parser_initialize(struct config* config)
{
    parser_config = config;
}

/**
 * If the declaration of yylex() does not exit, clang warns "implicait
 * declaration". yacc does not have any options to declare it. I dislike to add
 * this manually.
 */
extern int yylex();

static int
yyerror(const char* msg)
{
    fprintf(stderr, "parser error: %s\n", msg);
    return 0;   /* No one use this value? */
}
%}
%union {
    struct mapping* mapping;
    char* string;
}
/*
 * <machine/trap.h> on FreeBSD 9.1 defines T_USER. To avoid the compile error,
 * one underscore "_" is appended to T_USER.
 */
%token T_DAEMON T_END T_GROUP T_MAPPING T_NEWLINE T_USER_
%token<string> T_STRING
%type<mapping> mapping mappings
%%
config  : config T_NEWLINE section
        | section
        ;
section : T_MAPPING T_NEWLINE mappings T_NEWLINE T_END {
            parser_config->mappings = $3;
        }
        | T_DAEMON T_NEWLINE
                T_USER_ T_COLON T_STRING T_NEWLINE
                T_GROUP T_COLON T_STRING T_NEWLINE
                T_END {
            assert(strlen($5) < sizeof(parser_config->daemon.user));
            assert(strlen($9) < sizeof(parser_config->daemon.group));
            sprintf(parser_config->daemon.user, "%s", $5);
            sprintf(parser_config->daemon.group, "%s", $9);
        }
        | /* empty */
        ;
mappings
        : mappings T_NEWLINE mapping {
            $1->next = $3;
            $$ = $1;
        }
        | mapping {
            $$ = $1;
        }
        ;
mapping : T_STRING T_COLON T_STRING {
            assert(strlen($1) + 1 < sizeof($$->name));
            assert(strlen($3) + 1 < sizeof($$->path));
            $$ = memory_allocate(sizeof(*$$));
            strcpy($$->name, $1);
            strcpy($$->path, $3);
            $$->next = NULL;
        }
        | /* empty */ {
            $$ = NULL;
        }
        ;
%%
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
