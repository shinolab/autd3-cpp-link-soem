cmake_minimum_required(VERSION 3.21)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

project(autd3_link_soem VERSION 29.0.0)

message(STATUS "Build autd3_link_soem: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

option(USE_SYSTEM_EIGEN "Use system installed eigen3" OFF)
option(USE_SYSTEM_LIBCORO "Use system installed libcoro" OFF)
option(AUTD3_ASYNC_API "Use async APIs" OFF)
option(AUTD3_USE_PCH "Use precompiled headers" OFF)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0") 
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
if(WIN32)
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64|IA64")
    FetchContent_Declare(
      autd3
      URL https://github.com/shinolab/autd3-cpp/releases/download/v29.0.0-rc.8/autd3-v29.0.0-rc.8-win-x64.zip
    )
  elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "ARM64")
    FetchContent_Declare(
      autd3
      URL https://github.com/shinolab/autd3-cpp/releases/download/v29.0.0-rc.8/autd3-v29.0.0-rc.8-win-arm.zip
    )
  endif()
elseif(APPLE)
  FetchContent_Declare(
    autd3
    URL https://github.com/shinolab/autd3-cpp/releases/download/v29.0.0-rc.8/autd3-v29.0.0-rc.8-macos-aarch64.tar.gz
  )
else()
  FetchContent_Declare(
    autd3
    URL https://github.com/shinolab/autd3-cpp/releases/download/v29.0.0-rc.8/autd3-v29.0.0-rc.8-linux-x64.tar.gz
  )
endif()
FetchContent_MakeAvailable(autd3)

if(WIN32)
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64|IA64")
    set(WPCAP_LIB_DIR ${PROJECT_SOURCE_DIR}/lib/wpcap/x64)
  elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "ARM64")
    set(WPCAP_LIB_DIR ${PROJECT_SOURCE_DIR}/lib/wpcap/ARM64)
  endif()
endif()

add_library(autd3_link_soem INTERFACE)
target_include_directories(autd3_link_soem INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(autd3_link_soem INTERFACE autd3::common autd3capi_link_soem)
target_link_directories(autd3_link_soem INTERFACE ${PROJECT_SOURCE_DIR}/lib)
target_compile_definitions(autd3_link_soem INTERFACE AUTD3_EXT_TRACING)
if(AUTD3_USE_PCH)
  target_precompile_headers(autd3_link_soem REUSE_FROM autd3::common)
endif()
if(WIN32)
  target_link_libraries(autd3_link_soem INTERFACE Ws2_32.lib Winmm.lib)
  target_link_directories(autd3_link_soem INTERFACE ${WPCAP_LIB_DIR})
  target_link_libraries(autd3_link_soem INTERFACE wpcap.lib)
elseif(APPLE)
  target_link_libraries(autd3_link_soem INTERFACE pcap)
endif()
add_library(autd3::link::soem ALIAS autd3_link_soem)
