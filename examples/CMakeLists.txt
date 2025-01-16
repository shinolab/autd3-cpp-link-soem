cmake_minimum_required(VERSION 3.21)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

project(autd3-examples)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AUTD_LOCAL_TEST "For local test build" OFF)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0") 
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
if(AUTD_LOCAL_TEST)
  FetchContent_Declare(
    autd3_link_soem
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/..
  )
elseif(WIN32)
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64|IA64")
    FetchContent_Declare(
      autd3_link_soem
      URL https://github.com/shinolab/autd3-cpp-link-soem/releases/download/v29.0.0-rc.16/autd3-link-soem-v29.0.0-rc.16-win-x64.zip
    )
  elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "ARM64")
    FetchContent_Declare(
      autd3_link_soem
      URL https://github.com/shinolab/autd3-cpp-link-soem/releases/download/v29.0.0-rc.16/autd3-link-soem-v29.0.0-rc.16-win-arm.zip
    )
  endif()
elseif(APPLE)
  FetchContent_Declare(
    autd3_link_soem
    URL https://github.com/shinolab/autd3-cpp-link-soem/releases/download/v29.0.0-rc.16/autd3-link-soem-v29.0.0-rc.16-macos-aarch64.tar.gz
  )
else()
  FetchContent_Declare(
    autd3_link_soem
    URL https://github.com/shinolab/autd3-cpp-link-soem/releases/download/v29.0.0-rc.16/autd3-link-soem-v29.0.0-rc.16-linux-x64.tar.gz
  )
endif()
FetchContent_MakeAvailable(autd3_link_soem)

add_executable(soem soem.cpp)
target_link_libraries(soem PRIVATE autd3::autd3 autd3::link::soem)

add_executable(remote_soem remote_soem.cpp)
target_link_libraries(remote_soem PRIVATE autd3::autd3 autd3::link::soem)
