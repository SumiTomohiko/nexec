#if !defined(NEXEC_NEXECD_H_INCLUDED)
#define NEXEC_NEXECD_H_INCLUDED

#include <sys/param.h>

struct Mapping {
    struct Mapping* next;
    char name[32];
    char path[MAXPATHLEN];
};

struct Config {
    struct Mapping* mappings;
};

/* child_main.c */
void child_main(struct Config*, int);

/* memory.c */
void* memory_allocate(size_t);
void memory_dispose();
void memory_initialize();

/* parser.y */
void parser_initialize(struct Config*);

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
