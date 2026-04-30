cmake_minimum_required(VERSION 4.0)
project(Meshix)

set(CMAKE_CXX_STANDARD 20)

include(Vertix/cmake-modules/AddHlslShaders.cmake)

add_subdirectory(Vertix)
add_subdirectory(Meshix)
