cmake_minimum_required(VERSION 3.28)
project(grady_lib)

set(CMAKE_CXX_STANDARD 26)

if(APPLE)
    set(EXTRA_CXX_FLAGS "-DACCELERATE_NEW_LAPACK")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${EXTRA_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g ${EXTRA_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_PROFILE "-O3 -pg -g ${EXTRA_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_COVERAGE "-Og -g -fprofile-arcs -ftest-coverage ${EXTRA_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_SANADDR "-O3 -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer ${EXTRA_CXX_FLAGS}")

if (APPLE)
    set(EXTRA_LFLAGS "-framework Accelerate")
endif()

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

        src/gradylib/nn/Embedding.hpp
        src/gradylib/nn/FeatureIndexLookup.hpp
        src/gradylib/nn/GraphNode.hpp
        src/gradylib/nn/MergeLayer.hpp
        src/gradylib/nn/Linear.hpp
        src/gradylib/nn/Node.hpp
        src/gradylib/nn/Params.hpp
)

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

set(NN_SRC
        src/gradylib/nn4/activation/Scalarati.hpp
        src/gradylib/nn4/activation/Tensorati.hpp
        src/gradylib/nn4/DataType.hpp
        src/gradylib/nn4/DataTypeDeriver.hpp
        src/gradylib/nn4/Evaluator.hpp
        src/gradylib/nn4/Expression.hpp
        src/gradylib/nn4/Initializer.hpp
        src/gradylib/nn4/OutputBuffer.hpp
        src/gradylib/nn4/PrettyPrinter.hpp
        src/gradylib/nn4/Tensor.hpp
        src/gradylib/nn4/Trainer.hpp
        src/gradylib/nn4/Util.hpp
)

set(NN_TEST_SRC
        src/test/nn/TestMLP.cpp
        src/test/nn/TestScalar.cpp
)

add_executable(nnTests ${SRC} ${NN_SRC} ${NN_TEST_SRC})
target_link_libraries(nnTests PRIVATE Catch2 Catch2Main ${EXTRA_LFLAGS})

add_executable(replaceRegex src/experiment/ReplaceRegex.cpp)

add_executable(testNN ${SRC} src/test/nn/TestNN.cpp)
add_executable(testNN2 ${SRC} src/test/nn/TestNN2.cpp)
add_executable(testNNFeatureIndexer ${SRC} src/test/nn/testNNFeatureIndexer.cpp)

add_executable(externalPolymorphism ${SRC} src/experiment/ExternalPolymorphism.cpp)
add_executable(multipleInheritanceOfTemplate ${SRC} src/experiment/MultipleInheritanceOfTemplate.cpp)

add_executable(tensor ${SRC} src/gradylib/nn4/Tensor.cpp)
target_link_libraries(tensor ${EXTRA_LFLAGS})

add_executable(tensorInput ${SRC} src/gradylib/nn4/TensorInput.cpp)
target_link_libraries(tensorInput ${EXTRA_LFLAGS})
