
add_executable(nexecd main.c)
find_library(LIB_DIE die PATHS ${FSYSCALL_DIR}/lib/die)
find_library(LIB_START_MASTER start_master PATHS ${FSYSCALL_DIR}/lib/start_master)
target_link_libraries(nexecd ${LIB_START_MASTER} ${LIB_DIE})
