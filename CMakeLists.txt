
project(nexec)
set(NEXEC_VERSION \"1.0.0dev1\")
set(NEXEC_DEFAULT_PORT \"57005\")

cmake_minimum_required(VERSION 2.8)

option(FSYSCALL_DIR "fsyscall source directory" ${HOME}/projects/fsyscall2)

set(CMAKE_VERBOSE_MAKEFILE true)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -O3")
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
include_directories(${INCLUDE_DIR} ${FSYSCALL_DIR}/include)
configure_file(${INCLUDE_DIR}/nexec/config.h.in ${INCLUDE_DIR}/nexec/config.h)

subdirs(nexec nexecd)

add_custom_target(doc COMMAND cd doc && make)
add_custom_target(java COMMAND cd java && ant)
add_custom_target(java-clean COMMAND cd java && rm -rf bin)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
