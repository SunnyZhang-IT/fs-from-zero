cmake_minimum_required(VERSION 3.16)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17 -D_FILE_OFFSET_BITS=64 -D__coverage -I/usr/include/fuse3 -lfuse3 -lpthread")
project(CMakeSunny
    VERSION 1.0
    DESCRIPTION "A CMake Tutorial"
    LANGUAGES CXX)

add_executable(helloworld
    mem_fs.cpp
    memoryFS.cpp
    Fuse.cpp)

target_link_libraries(helloworld -lfuse3)
