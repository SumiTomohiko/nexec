#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <nexec/util.h>

void
die(const char* fmt, ...)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s\n", fmt);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, buf, ap);
    va_end(ap);

    va_start(ap, fmt);
    vsyslog(LOG_ERR, fmt, ap);
    va_end(ap);

    exit(1);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
