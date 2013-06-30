%{
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <nexec/nexecd.h>

static struct Config* parser_config;

void
parser_initialize(struct Config* config)
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
    struct Mapping* mapping;
    char* string;
}
%token T_END T_MAPPING T_NEWLINE
%token<string> T_STRING
%type<mapping> mapping mappings
%%
config  : config T_NEWLINE section
        | section
        ;
section : T_MAPPING T_NEWLINE mappings T_NEWLINE T_END {
            parser_config->mappings = $3;
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
