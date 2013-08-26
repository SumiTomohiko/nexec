#if !defined(NEXEC_UTIL_H_INCLUDED)
#define NEXEC_UTIL_H_INCLUDED

void die(const char*, ...);
void read_line(int, char*, size_t);
void set_tcp_nodelay_or_die(int);
void writeln(int, const char*);

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
