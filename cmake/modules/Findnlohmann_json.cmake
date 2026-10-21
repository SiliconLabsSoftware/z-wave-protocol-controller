# nlohmann/json via FetchContent (header-only), same pattern as FindSQLite3.cmake.
# Release tarball + URL_HASH instead of a git clone.
#
# Target:
#   nlohmann_json::nlohmann_json

if(TARGET nlohmann_json::nlohmann_json)
  set(nlohmann_json_FOUND TRUE)
  return()
endif()

message(NOTICE "-- Finding nlohmann_json (FetchContent header-only)")

include(FetchContent)
include(${CMAKE_CURRENT_LIST_DIR}/../include/external_compiler_flags.cmake)
set(FETCHCONTENT_QUIET TRUE)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")

set(nlohmann_json_VERSION 3.12.0)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa)
zpc_push_external_compiler_flags()
FetchContent_MakeAvailable(nlohmann_json)
zpc_pop_external_compiler_flags()

if(NOT TARGET nlohmann_json::nlohmann_json)
  message(FATAL_ERROR "Failed to build nlohmann_json from source.")
endif()

set(nlohmann_json_FOUND TRUE)
