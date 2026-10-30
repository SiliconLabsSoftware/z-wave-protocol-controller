cmake_minimum_required(VERSION 3.21)
# The coverage preset selects Clang by default. Explicit compiler selections
# (including CI's versioned CC/CXX) still take precedence. macOS already defaults to Clang.
if(COVERAGE AND NOT CMAKE_HOST_APPLE)
  if(NOT DEFINED CMAKE_C_COMPILER AND "$ENV{CC}" STREQUAL "")
    set(CMAKE_C_COMPILER clang CACHE FILEPATH "C compiler")
  endif()
  if(NOT DEFINED CMAKE_CXX_COMPILER AND "$ENV{CXX}" STREQUAL "")
    set(CMAKE_CXX_COMPILER clang++ CACHE FILEPATH "C++ compiler")
  endif()
endif()
project(z-wave-protocol-controller)

# Presets map CMAKE_INSTALL_PREFIX from $penv{}; unset → empty → restore CMake default.
if(CMAKE_INSTALL_PREFIX STREQUAL "")
  set(CMAKE_INSTALL_PREFIX
      "/usr/local"
      CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
endif()

# Set the log level to NOTICE on local builds
if(NOT DEFINED ENV{CI})
  set(CMAKE_MESSAGE_LOG_LEVEL NOTICE)
endif()
# ##############################################################################
# FetchContent Configuration (for external dependencies)
# ##############################################################################
# Configure FetchContent to cache downloads and avoid re-downloading
# This speeds up configuration time significantly
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE CACHE BOOL "Don't update external dependencies if already populated")
set(FETCHCONTENT_QUIET TRUE)

# ##############################################################################
# Include Configurations
# ##############################################################################
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.4")
  cmake_diagnostic(SET CMD_DEPRECATED WARN RECURSE)
else()
  set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "Enable warnings for deprecated CMake commands and variables")
endif() 
option(COVERAGE "Enable coverage reporting" OFF)
if(COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Coverage requires Clang. Remove a conflicting compiler override and configure again with --fresh.")
    endif()
    message(STATUS "Building with LLVM source-based coverage")
    add_compile_options(
      -fprofile-instr-generate
      -fcoverage-mapping
      -fprofile-list=${CMAKE_SOURCE_DIR}/ci/coverage/llvm_profile_list.txt
      -O0 -g)
    add_link_options(-fprofile-instr-generate)
    # A configure starts a new measurement. Run all tests before reporting.
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/coverage")
endif()

# Include compiler options early (before other includes that might use compiler settings)
# compiler_options.cmake will set default build type to Debug if not specified
include(cmake/include/compiler_options.cmake)
include(cmake/include/version.cmake)

# CTest + unit test helpers (debug / release presets leave UNIT_TEST off; unit-test preset turns it on)
include(CTest)
option(UNIT_TEST "Configure unit tests" OFF)
if(UNIT_TEST)
  include(cmake/include/unity_test.cmake)
  include(cmake/include/gtest_test.cmake)
  add_test(NAME coverage_reporting
    COMMAND ${Python3_EXECUTABLE} -m unittest discover
      -s ${CMAKE_SOURCE_DIR}/ci/scripts -p test_check_coverage.py)
endif()


# ##############################################################################
# Global includes
# ##############################################################################
# Note: Common headers are now provided via the 'common' component
# Generated headers (like zpc_version.h) are in ${CMAKE_CURRENT_BINARY_DIR}/include
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

# ##############################################################################
# Subdirectories
# #############################################################################
add_subdirectory(applications)

# ##############################################################################
# Extra targets
# ##############################################################################

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR UNIT_TEST)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)

  set(ZPC_PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
  if(EXISTS "${CMAKE_SOURCE_DIR}/.venv/bin/python")
    set(ZPC_PYTHON_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/bin/python")
  elseif(EXISTS "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe")
    set(ZPC_PYTHON_EXECUTABLE "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe")
  endif()

  add_custom_target(
    merge_compile_commands
    COMMAND "${ZPC_PYTHON_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/ci/scripts/merge_compile_commands.py"
    COMMENT "Merging Debug and unit-test compilation databases"
    VERBATIM)

  if(TARGET zpc)
    add_dependencies(zpc merge_compile_commands)
  endif()
  if(TARGET UNIT_TEST)
    add_dependencies(UNIT_TEST merge_compile_commands)
  endif()
endif()

include(cmake/include/version_file.cmake)

# Debian .deb packaging (CPack); Linux hosts only — see docs/packaging-debian.md
if(CMAKE_HOST_UNIX AND NOT APPLE)
  include(cmake/include/cpack.cmake)
endif()
