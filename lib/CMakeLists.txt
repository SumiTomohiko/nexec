
add_library(util STATIC read_line.c writeln.c alloc_env_or_die.c sslutil.c)
add_library(set_tcp_nodelay_or_die STATIC set_tcp_nodelay_or_die.c)
