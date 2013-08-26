
add_executable(nexec main.c)
find_library(LIB_CLOSE_OR_DIE close_or_die PATHS ${FSYSCALL_DIR}/lib/close_or_die)
find_library(LIB_DIE die PATHS ${FSYSCALL_DIR}/lib/die)
find_library(LIB_FORK_OR_DIE fork_or_die PATHS ${FSYSCALL_DIR}/lib/fork_or_die)
find_library(LIB_PIPE_OR_DIE pipe_or_die PATHS ${FSYSCALL_DIR}/lib/pipe_or_die)
find_library(LIB_START_SLAVE start_slave PATHS ${FSYSCALL_DIR}/lib/start_slave)
target_link_libraries(
    nexec
    util set_tcp_nodelay_or_die ${LIB_START_SLAVE} ${LIB_CLOSE_OR_DIE}
    ${LIB_PIPE_OR_DIE} ${LIB_FORK_OR_DIE} ${LIB_DIE})
install(PROGRAMS nexec DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
