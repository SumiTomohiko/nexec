#if !defined(NEXEC_UTIL_H_INCLUDED)
#define NEXEC_UTIL_H_INCLUDED

void die(const char*, ...);
void setblock(int);
void setnonblock(int);
void read_line(int, char*, size_t);
void writeln(int, const char*);

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
