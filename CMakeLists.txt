cmake_minimum_required(VERSION 3.28)
project(grady_lib)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g")
set(CMAKE_CXX_FLAGS_PROFILE "-O3 -pg -g")
set(CMAKE_CXX_FLAGS_COVERAGE "-Og -g -fprofile-arcs -ftest-coverage")

include_directories(
        src/
        ../Catch2/src/
        ../Catch2/release/generated-includes/
)

link_directories(
        ../Catch2/release/src/
)

set(SRC
        src/gradylib/AltIntHash.hpp
        src/gradylib/BitPairSet.hpp
        src/gradylib/CompletionPool.hpp
        src/gradylib/MMapI2HRSOpenHashMap.hpp
        src/gradylib/MMapI2SOpenHashMap.hpp
        src/gradylib/MMapS2IOpenHashMap.hpp
        src/gradylib/OpenHashMap.hpp
        src/gradylib/OpenHashMapTC.hpp
        src/gradylib/OpenHashSet.hpp
        src/gradylib/OpenHashSetTC.hpp
        src/gradylib/ThreadPool.hpp
        src/gradylib/ParallelTraversals.hpp
)

add_executable(testOpenHashSetTC ${SRC} src/test/TestOpenHashSetTC.cpp)
add_executable(testOpenHashMapTC ${SRC} src/test/TestOpenHashMapTC.cpp)
add_executable(testOpenHashMap ${SRC} src/test/TestOpenHashMap.cpp)
add_executable(testOpenHashSet ${SRC} src/test/TestOpenHashSet.cpp)
add_executable(testMMapViewableOpenHashMap ${SRC} src/test/TestMMapViewableOpenHashMap.cpp)
add_executable(testAlignment ${SRC} src/test/AlignmentStuff.cpp)
add_executable(perfectForwardStuff ${SRC} src/test/PerfectForwardStuff.cpp)
add_executable(testCompletionPool ${SRC} src/test/TestCompletionPool.cpp)
add_executable(testMMapI2HRSOpenHashMap ${SRC} src/test/TestMMapI2HRSOpenHashMap.cpp)
add_executable(testParallelMapOperation ${SRC} src/test/TestParallelMapOperation.cpp)
add_executable(threadPoolTest ${SRC} src/test/ThreadPoolTest.cpp)
add_executable(testStringViewKeys ${SRC} src/test/TestStringViewKeys.cpp)
add_executable(testAltIntHash ${SRC} src/test/TestAltIntHash.cpp)

add_executable(testParallelTraversals ${SRC} src/test/TestParallelTraversals.cpp)
target_link_libraries(testParallelTraversals PRIVATE Catch2 Catch2Main)

add_executable(testOpenHashMapTC2 ${SRC} src/test/TestOpenHashMapTC2.cpp)
target_link_libraries(testOpenHashMapTC2 PRIVATE Catch2 Catch2Main)
