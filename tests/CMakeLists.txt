cmake_minimum_required(VERSION 3.21)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

project(autd3-test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(COVERAGE "Take coverage" OFF)

if(COVERAGE)
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O0 -g --coverage -fno-inline -fno-inline-small-functions -fno-default-inline -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage -fprofile-arcs -fprofile-generate")
endif()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
include(${PROJECT_SOURCE_DIR}/gtest.cmake)

enable_testing()

include(FetchContent)
FetchContent_Declare(
  autd3_link_soem
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/..
)
FetchContent_MakeAvailable(autd3_link_soem)

add_executable(test_autd3 main.cpp)

target_include_directories(test_autd3 PRIVATE .)
target_link_libraries(test_autd3 PRIVATE gtest_main)
target_link_libraries(test_autd3 PRIVATE autd3::autd3 autd3::link::soem)

add_test(NAME test_autd3 COMMAND test_autd3)
target_sources(test_autd3 PRIVATE
  soem.cpp
)

include(GoogleTest)
gtest_discover_tests(test_autd3)

if(WIN32)
  target_link_options(test_autd3 PRIVATE /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib)
endif()

if(COVERAGE)
  target_compile_definitions(test_autd3 PRIVATE COVERAGE)
endif()
