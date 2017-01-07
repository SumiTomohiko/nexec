#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <nexec/util.h>

struct env*
alloc_env_or_die()
{
    struct env* penv = (struct env*)malloc(sizeof(*penv));
    if (penv == NULL) {
        die("cannot allocate memory for struct env: %s", strerror(errno));
    }
    return penv;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
