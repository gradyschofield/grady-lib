cmake_minimum_required(VERSION 3.28)
project(grady_lib)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_PROFILE "-O3 -p")

include_directories(src/)

set(SRC
        src/AltIntHash.hpp
        src/BitPairSet.hpp
        src/MMapI2HRSOpenHashMap.hpp
        src/MMapI2SOpenHashMap.hpp
        src/MMapS2IOpenHashMap.hpp
        src/OpenHashMap.hpp
        src/OpenHashMapTC.hpp
        src/OpenHashSetTC.hpp
        src/ThreadPool.hpp
        src/ParallelTraversals.hpp
)

add_executable(testOpenHashSetTC ${SRC} src/test/TestOpenHashSetTC.cpp)
add_executable(testOpenHashMapTC ${SRC} src/test/TestOpenHashMapTC.cpp)
add_executable(testOpenHashMap ${SRC} src/test/TestOpenHashMap.cpp)
add_executable(testMMapViewableOpenHashMap ${SRC} src/test/TestMMapViewableOpenHashMap.cpp)
add_executable(testAlignment ${SRC} src/test/AlignmentStuff.cpp)
add_executable(perfectForwardStuff ${SRC} src/test/PerfectForwardStuff.cpp)
add_executable(testMMapI2HRSOpenHashMap ${SRC} src/test/TestMMapI2HRSOpenHashMap.cpp)
add_executable(testParallelMapOperation ${SRC} src/test/TestParallelMapOperation.cpp)
add_executable(threadPoolTest ${SRC} src/test/ThreadPoolTest.cpp)
add_executable(testParallelTraversals ${SRC} src/test/TestParallelTraversals.cpp)
add_executable(testAltIntHash ${SRC} src/test/TestAltIntHash.cpp)
