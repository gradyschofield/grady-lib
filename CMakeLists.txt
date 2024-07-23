cmake_minimum_required(VERSION 3.28)
project(grady_lib)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

include_directories(src/)

set(SRC
        src/BitPairSet.hpp
        src/OpenHashSetTC.hpp
        src/OpenHashMap.hpp
        src/OpenHashMapTC.hpp)

add_executable(testOpenHashSet ${SRC} src/main.cpp)
add_executable(testOpenHashMapTC ${SRC} src/test/TestOpenHashMapTC.cpp)
add_executable(testOpenHashMap ${SRC} src/test/TestOpenHashMap.cpp)
