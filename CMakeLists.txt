cmake_minimum_required(VERSION 3.24)
project(DTMFKeys)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0)

link_directories(${GTKMM_LIBRARY_DIRS})
include_directories(${GTKMM_INCLUDE_DIRS})

add_executable(DTMFKeys main.cpp)
target_link_libraries(DTMFKeys SDL2 ${GTKMM_LIBRARIES})
