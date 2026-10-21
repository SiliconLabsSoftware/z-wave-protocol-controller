# yaml-cpp via FetchContent (static), same pattern as FindFMT.cmake.
#
# Target:
#   yaml-cpp::yaml-cpp

if(TARGET yaml-cpp::yaml-cpp)
  set(Yaml-cpp_FOUND TRUE)
  return()
endif()

message(NOTICE "-- Finding yaml-cpp (FetchContent static)")

include(FetchContent)
include(${CMAKE_CURRENT_LIST_DIR}/../include/external_compiler_flags.cmake)
set(FETCHCONTENT_QUIET TRUE)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "Build yaml-cpp as a shared library" FORCE)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Build yaml-cpp tests" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Build yaml-cpp tools" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Build yaml-cpp contrib" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "Install yaml-cpp" FORCE)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG        0.8.0
  GIT_SHALLOW    TRUE)
# yaml-cpp 0.8.0 still declares cmake_minimum_required < 3.5.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
zpc_push_external_compiler_flags()
FetchContent_MakeAvailable(yaml-cpp)
zpc_pop_external_compiler_flags()

if(NOT TARGET yaml-cpp::yaml-cpp)
  if(TARGET yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
  else()
    message(FATAL_ERROR "Failed to build yaml-cpp from source. Could not find yaml-cpp target.")
  endif()
endif()

set(Yaml-cpp_FOUND TRUE)
set(YAML_CPP_LIBRARIES yaml-cpp::yaml-cpp)
