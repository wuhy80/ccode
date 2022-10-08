cmake_minimum_required(VERSION 3.22)

project(test)

find_package (Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++17")
set(source_dir "${PROJECT_SOURCE_DIR}/src/")

FILE(GLOB source_files "${source_dir}/main.cpp")

add_executable(test ${source_files})
target_link_libraries (test ${CMAKE_THREAD_LIBS_INIT})
