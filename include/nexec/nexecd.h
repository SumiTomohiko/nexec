#if !defined(NEXEC_NEXECD_H_INCLUDED)
#define NEXEC_NEXECD_H_INCLUDED

#include <sys/param.h>

struct daemon {
    char user[16];
    char group[16];
};

struct mapping {
    struct mapping* next;
    char name[32];
    char path[MAXPATHLEN];
};

struct config {
    struct daemon daemon;
    struct mapping* mappings;
};

/* child_main.c */
void child_main(struct config*, int);

/* memory.c */
void* memory_allocate(size_t);
void memory_dispose();
void memory_initialize();

/* parser.y */
void parser_initialize(struct config*);

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
