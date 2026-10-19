# SQLite 3 — official amalgamation via FetchContent (static), same pattern as
# FindFMT.cmake and FindPahoMQTTCpp.cmake.
#
# Defines:
#   SQLite3_FOUND, SQLite3_VERSION, SQLite3_INCLUDE_DIRS, SQLite3_INCLUDE_DIR,
#   SQLite3_LIBRARIES
# Target:
#   sqlite3_amalgamation (STATIC), also available as SQLite::SQLite3 (ALIAS).

if(TARGET sqlite3_amalgamation)
  get_target_property(_sqlite_inc sqlite3_amalgamation INTERFACE_INCLUDE_DIRECTORIES)
  set(SQLite3_INCLUDE_DIR "${_sqlite_inc}")
  set(SQLite3_INCLUDE_DIRS "${_sqlite_inc}")
  set(SQLite3_LIBRARIES SQLite::SQLite3)
  set(SQLite3_VERSION 3.53.0)
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(
    SQLite3
    REQUIRED_VARS SQLite3_INCLUDE_DIR SQLite3_LIBRARIES
    VERSION_VAR SQLite3_VERSION)
  unset(_sqlite_inc)
  return()
endif()

message(NOTICE "-- Finding SQLite3 (FetchContent amalgamation)")

# Pinned release; bump _SQLITE_AMALG_VER / URL / URL_HASH / SQLite3_VERSION together.
set(_SQLITE_AMALG_VER 3530000)
set(SQLite3_VERSION 3.53.0)

include(FetchContent)
include(${CMAKE_CURRENT_LIST_DIR}/../include/external_compiler_flags.cmake)
set(FETCHCONTENT_QUIET TRUE)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

FetchContent_Declare(
  sqlite_amalgamation
  URL "https://www.sqlite.org/2026/sqlite-amalgamation-${_SQLITE_AMALG_VER}.zip"
  URL_HASH SHA256=bf3733d7c71b3ab0f6fd8a9ea0052ad87fa037d94333e14ce09878ba3492c3b0)

zpc_push_external_compiler_flags()
FetchContent_MakeAvailable(sqlite_amalgamation)

set(_sqlite_src "${sqlite_amalgamation_SOURCE_DIR}")
if(NOT EXISTS "${_sqlite_src}/sqlite3.c")
  file(GLOB _sqlite3_c "${_sqlite_src}/*/sqlite3.c")
  if(_sqlite3_c)
    get_filename_component(_sqlite_src "${_sqlite3_c}" DIRECTORY)
  endif()
endif()
if(NOT EXISTS "${_sqlite_src}/sqlite3.c")
  message(
    FATAL_ERROR
      "SQLite amalgamation: sqlite3.c not found under ${sqlite_amalgamation_SOURCE_DIR}")
endif()

set(ZPC_SQLITE3_C "${_sqlite_src}/sqlite3.c")
set(ZPC_SQLITE3_DIR "${_sqlite_src}")
add_subdirectory(
  ${CMAKE_CURRENT_LIST_DIR}/../external/sqlite
  ${CMAKE_BINARY_DIR}/_deps/zpc-sqlite3-build)
zpc_pop_external_compiler_flags()

add_library(SQLite::SQLite3 ALIAS sqlite3_amalgamation)

set(SQLite3_INCLUDE_DIR "${_sqlite_src}")
set(SQLite3_INCLUDE_DIRS "${_sqlite_src}")
set(SQLite3_LIBRARIES SQLite::SQLite3)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  SQLite3
  REQUIRED_VARS SQLite3_INCLUDE_DIR SQLite3_LIBRARIES
  VERSION_VAR SQLite3_VERSION)

unset(_SQLITE_AMALG_VER)
unset(_sqlite_src)
unset(_sqlite3_c)
