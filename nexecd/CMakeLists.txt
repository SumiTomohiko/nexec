
add_executable(nexecd main.c child_main.c)
find_library(LIB_START_MASTER start_master PATHS ${FSYSCALL_DIR}/lib/start_master)
target_link_libraries(nexecd ${LIB_START_MASTER} ${LIB_DIE})
