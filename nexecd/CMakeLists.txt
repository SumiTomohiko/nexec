
add_custom_command(
    OUTPUT parser.c parser.h parser.output
    COMMAND yacc -dv -o parser.c parser.y
    DEPENDS parser.y)
add_custom_command(OUTPUT lexer.c COMMAND lex -olexer.c lexer.l DEPENDS lexer.l)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DYY_NO_UNPUT -DYY_NO_INPUT -DYYDEBUG")
add_executable(nexecd main.c child_main.c parser.c lexer.c memory.c)
find_library(LIB_START_MASTER start_master PATHS ${FSYSCALL_DIR}/lib/start_master)
target_link_libraries(nexecd util ${LIB_START_MASTER} ${LIB_DIE})
install(PROGRAMS nexecd DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
