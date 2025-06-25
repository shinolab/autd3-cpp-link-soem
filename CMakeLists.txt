cmake_minimum_required(VERSION 3.21)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

project(autd3_link_soem VERSION 34.0.0)

message(STATUS "Build autd3_link_soem: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

option(USE_SYSTEM_EIGEN "Use system installed eigen3" OFF)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0") 
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
if(WIN32)
  FetchContent_Declare(
    autd3
    URL https://github.com/shinolab/autd3-cpp/releases/download/v34.0.0/autd3-v34.0.0-win-x64.zip
  )
elseif(APPLE)
  FetchContent_Declare(
    autd3
    URL https://github.com/shinolab/autd3-cpp/releases/download/v34.0.0/autd3-v34.0.0-macos-aarch64.tar.gz
  )
else()
  FetchContent_Declare(
    autd3
    URL https://github.com/shinolab/autd3-cpp/releases/download/v34.0.0/autd3-v34.0.0-linux-x64.tar.gz
  )
endif()
FetchContent_MakeAvailable(autd3)

macro(autd_target_link_libraries target lib)
  if(WIN32)
    target_link_libraries(${target} INTERFACE "${lib}.dll.lib")
  elseif(APPLE)
    target_link_libraries(${target} INTERFACE ${lib})
  else()
    target_link_libraries(${target} INTERFACE ${lib})
  endif()
endmacro()

add_library(autd3_link_soem INTERFACE)
target_include_directories(autd3_link_soem INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(autd3_link_soem INTERFACE autd3::common)
autd_target_link_libraries(autd3_link_soem autd3capi_link_soem)
if(WIN32)
  target_link_directories(autd3_link_soem INTERFACE ${PROJECT_SOURCE_DIR}/lib)
else()
  target_link_directories(autd3_link_soem INTERFACE ${PROJECT_SOURCE_DIR}/bin)
endif()
if(WIN32)
  target_link_libraries(autd3_link_soem INTERFACE Ws2_32.lib Winmm.lib)
  target_link_directories(autd3_link_soem INTERFACE ${PROJECT_SOURCE_DIR}/lib/wpcap/x64)
  target_link_libraries(autd3_link_soem INTERFACE wpcap.lib)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  add_custom_target(autd3capi_link_soem)
  add_custom_command(TARGET autd3capi_link_soem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/$<CONFIG>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/bin/autd3capi_link_soem.dll" ${CMAKE_BINARY_DIR}/$<CONFIG>
    VERBATIM
  )
  set_property(TARGET autd3capi_link_soem PROPERTY FOLDER "autd3capi") 
  add_dependencies(autd3_link_soem autd3capi_link_soem)
elseif(APPLE)
  target_link_libraries(autd3_link_soem INTERFACE pcap)
endif()
add_library(autd3::link::soem ALIAS autd3_link_soem)
