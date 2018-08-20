cmake_minimum_required(VERSION 2.6)
project(fmtexpr)

set(CMAKE_CXX_STANDARD 17)

add_executable(fmtexpr main.cpp)
include_directories(.)

install(TARGETS fmtexpr RUNTIME DESTINATION bin)
