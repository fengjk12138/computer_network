cmake_minimum_required(VERSION 3.16)
project(network)

set(CMAKE_CXX_STANDARD 14)
link_libraries(ws2_32)
add_executable(network main.cpp)