cmake_minimum_required(VERSION 3.0.2)

PROJECT(SimpleDB)

option(ENABLE_TEST "Enable test program build" ON)

set(CMAKE_CXX_FLAGS "--std=c++11 -O3 ${CMAKE_CXX_FLAGS}")

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(lib/googletest)
include_directories(${gtest_SOURCE_DIR}/include)

add_subdirectory(src)

if (ENABLE_TEST)
    enable_testing()
    if (APPLE)
        add_definitions(-DGTEST_USE_OWN_TR1_TUPLE)
        add_definitions(-D__GLIBCXX__)
    endif (APPLE)
    include_directories(${gtest_SOURCE_DIR})
    add_subdirectory(test)
endif (ENABLE_TEST)