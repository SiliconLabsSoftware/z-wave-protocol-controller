cmake_minimum_required(VERSION 3.21)
project(z-wave-protocol-controller)

# debian preset maps CMAKE_INSTALL_PREFIX from $penv{}; unset → empty → restore CMake default.
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
if(COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Building with code coverage")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

# Include compiler options early (before other includes that might use compiler settings)
# compiler_options.cmake will set default build type to Debug if not specified
include(cmake/include/compiler_options.cmake)
include(cmake/include/version.cmake)

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

include(cmake/include/version_file.cmake)

# Debian .deb packaging (CPack); Linux hosts only — see docs/packaging-debian.md
if(CMAKE_HOST_UNIX AND NOT APPLE)
  include(cmake/include/cpack.cmake)
endif()
