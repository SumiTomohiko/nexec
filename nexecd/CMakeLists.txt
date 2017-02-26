
add_custom_command(OUTPUT parser.c parser.h parser.output
                   COMMAND yacc -dv -o parser.c parser.y
                   DEPENDS parser.y)
add_custom_command(OUTPUT lexer.c COMMAND lex -olexer.c lexer.l DEPENDS lexer.l)
add_custom_command(OUTPUT cmdlexer.c
                   COMMAND re2c -bcd -t cmdlexer.h -o cmdlexer.c cmdlexer.re
                   DEPENDS cmdlexer.re)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DYY_NO_UNPUT -DYY_NO_INPUT -DYYDEBUG")
add_executable(nexecd main.c child_main.c parser.c lexer.c cmdlexer.c memory.c)
add_executable(lexertest cmdlexer.c memory.c)
target_compile_options(lexertest PRIVATE -DLEXERTEST -Wno-unused-parameter)
find_library(LIB_DIE die PATHS ${FSYSCALL_DIR}/lib/die)
find_library(LIB_RUN_MASTER run_master PATHS ${FSYSCALL_DIR}/lib/run_master)
find_library(LIB_FMHUB fmhub PATHS ${FSYSCALL_DIR}/libfmhub)
target_link_libraries(nexecd ssl crypto util set_tcp_nodelay_or_die
                      ${LIB_RUN_MASTER} ${LIB_FMHUB} ${LIB_DIE})
install(PROGRAMS nexecd DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
