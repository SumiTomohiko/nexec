#if !defined(NEXEC_UTIL_H_INCLUDED)
#define NEXEC_UTIL_H_INCLUDED

struct env {
    struct env* next;
    const char* name;
    const char* value;
};

struct env* alloc_env_or_die();
void die(const char*, ...);
void read_line(int, char*, size_t);
void set_tcp_nodelay_or_die(int);
void writeln(int, const char*);

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
