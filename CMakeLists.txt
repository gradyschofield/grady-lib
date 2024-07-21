cmake_minimum_required(VERSION 3.28)
project(grady_lib)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g")

include_directories(src/)

set(SRC
        src/OpenHashSetOfTriviallyCopyables.hpp)

add_executable(testOpenHashSetOfTriviallyCopyables ${SRC} src/main.cpp)
