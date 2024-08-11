cmake_minimum_required(VERSION 3.28)
project(grady_lib)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_PROFILE "-O3 -pg -g")
set(CMAKE_CXX_FLAGS_COVERAGE "-Og -g -fprofile-arcs -ftest-coverage")
set(CMAKE_CXX_FLAGS_SANADDR "-O3 -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")

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

        src/gradylib/nn/MLP.hpp
        src/gradylib/nn/Node.hpp
)

<<<<<<< HEAD
add_executable(testAlignment ${SRC} src/experiment/AlignmentStuff.cpp)
add_executable(perfectForwardStuff ${SRC} src/experiment/PerfectForwardStuff.cpp)
add_executable(testAltIntHash ${SRC} src/experiment/TestAltIntHash.cpp)

add_executable(testStringViewKeys ${SRC} src/experiment/TestStringViewKeys.cpp)

set(TEST_SRC
        src/test/TestBitPairSet.cpp
        src/test/TestCompletionPool.cpp
        src/test/TestOpenHashMap.cpp
        src/test/TestMMapViewableOpenHashMap.cpp
        src/test/TestMMapI2HRSOpenHashMap.cpp
        src/test/ThreadPoolTest.cpp
        src/test/TestParallelTraversals.cpp
        src/test/TestOpenHashMapTC.cpp
        src/test/TestOpenHashMapTC2.cpp
        src/test/TestOpenHashSet.cpp
        src/test/TestOpenHashSetTC.cpp
)

add_executable(allTests ${SRC} ${TEST_SRC})
target_link_libraries(allTests PRIVATE Catch2 Catch2Main)

add_executable(replaceRegex src/experiment/ReplaceRegex.cpp)

add_executable(testNN ${SRC} src/test/TestNN.cpp)
