cmake_minimum_required(VERSION 3.28)
project(gradylib
        VERSION 0.1
        LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(gradylib INTERFACE)
add_library(gradylib::all ALIAS gradylib)

set_target_properties(gradylib PROPERTIES EXPORT_NAME all)

target_compile_features(gradylib INTERFACE cxx_std_23)

target_include_directories(gradylib
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
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
        src/gradylib/SparseDirect.hpp
        src/gradylib/SparseLeastSquares.hpp
        src/gradylib/SparseMatrix.hpp
        src/gradylib/SparseSolvers.hpp
        src/gradylib/ThreadPool.hpp
        src/gradylib/ParallelTraversals.hpp
)

#=========================================================================================================
# TESTS
#=========================================================================================================

include(CTest)

if(BUILD_TESTING)
    find_package(Catch2 3 REQUIRED)

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

    add_executable(gradylib-tests ${TEST_SRC})
    target_compile_options(gradylib-tests
            PRIVATE
            $<$<CONFIG:Profile>:-O3;-pg;-g>
            $<$<CONFIG:Coverage>:-Og;-g;-fprofile-arcs;-ftest-coverage>
            $<$<CONFIG:Sanaddr>:-O3;-g;-fsanitize=address;-fsanitize=undefined;-fno-omit-frame-pointer>
    )
    target_link_options(gradylib-tests
            PRIVATE
            $<$<CONFIG:Profile>:-pg>
            $<$<CONFIG:Coverage>:-fprofile-arcs>
            $<$<CONFIG:Sanaddr>:-fsanitize=address;-fsanitize=undefined>
    )
    target_link_libraries(
            gradylib-tests
            PRIVATE
            gradylib::all
            Catch2::Catch2WithMain
    )

    include(Catch)
    catch_discover_tests(gradylib-tests)
endif()

#=========================================================================================================
# INSTALLATION
#=========================================================================================================

install(
        TARGETS gradylib
        EXPORT gradylibTargets
)

install(
        DIRECTORY src/gradylib/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gradylib
        FILES_MATCHING PATTERN "*.hpp"
)

install(
        EXPORT gradylibTargets
        FILE gradylibTargets.cmake
        NAMESPACE grady::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gradylib
)

configure_package_config_file(
        cmake/gradylibConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/gradylibConfig.cmake"
        INSTALL_DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/gradylib
)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/gradylibConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/gradylibConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/gradylibConfigVersion.cmake"
        DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/gradylib
)

#=========================================================================================================
# EXPERIMENTAL PROGRAMS
#=========================================================================================================

option(GRADYLIB_BUILD_EXPERIMENTS "Build experiments" OFF)
if (GRADYLIB_BUILD_EXPERIMENTS)
    add_executable(testAlignment ${SRC} src/experiment/AlignmentStuff.cpp)
    add_executable(perfectForwardStuff ${SRC} src/experiment/PerfectForwardStuff.cpp)
    add_executable(testAltIntHash ${SRC} src/experiment/TestAltIntHash.cpp)
    add_executable(testStringViewKeys ${SRC} src/experiment/TestStringViewKeys.cpp)
    add_executable(replaceRegex src/experiment/ReplaceRegex.cpp)
endif()

