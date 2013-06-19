#if !defined(NEXEC_UTIL_H_INCLUDED)
#define NEXEC_UTIL_H_INCLUDED

void die(const char*, ...);
void setnonblock(int);
void read_line(int, char*, size_t);
void write_all(int, const void*, size_t);

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
