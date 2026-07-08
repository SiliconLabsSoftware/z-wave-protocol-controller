if(NOT COMPILER_OPTIONS)
  set(COMPILER_OPTIONS True)
else()
  return()
endif()

# ##############################################################################
# Build Type Configuration
# ##############################################################################
# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
  message(STATUS "No build type specified, defaulting to Debug")
endif()

# ##############################################################################
# C/Cpp Version Check
# ##############################################################################
# Supported: GCC 14.x, Clang 17.x, Clang 21.x (including AppleClang with the same majors).
# All other compilers fail configure.
string(REGEX MATCH "^([0-9]+)" _zpc_cxx_version_major "${CMAKE_CXX_COMPILER_VERSION}")
if(NOT CMAKE_MATCH_1)
  message(
    FATAL_ERROR
      "Could not parse ${CMAKE_CXX_COMPILER_ID} version from \"${CMAKE_CXX_COMPILER_VERSION}\""
  )
endif()
set(_zpc_cxx_major "${CMAKE_MATCH_1}")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(_zpc_cxx_major EQUAL 14)
    # supported
  else()
    message(
      FATAL_ERROR
        "Unsupported GCC ${CMAKE_CXX_COMPILER_VERSION}. Supported: GCC 14.x"
    )
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if(_zpc_cxx_major EQUAL 17 OR _zpc_cxx_major EQUAL 21)
    # supported
  else()
    message(
      FATAL_ERROR
        "Unsupported ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}. Supported: Clang 17.x, Clang 21.x"
    )
  endif()
else()
  message(
    FATAL_ERROR
      "Unsupported compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}. Supported: GCC 14.x, Clang 17.x, Clang 21.x"
  )
endif()
unset(_zpc_cxx_major)
unset(_zpc_cxx_version_major)

# ##############################################################################
# Compiler Options
# ##############################################################################
# Setup C/CPP Standard
set(CMAKE_C_STANDARD 99) # c99
set(CMAKE_C_EXTENSIONS ON) # Enable gnu99 extensions
set(CMAKE_CXX_STANDARD 20) # C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Do not allow fallback to previous C++
                                    # standards if compiler doesn't support it
set(CMAKE_CXX_EXTENSIONS ON) # Enable gnu++11 extentions

# ##############################################################################
# Build Type Specific Compiler Flags
# ##############################################################################
# Common flags for all build types
set(COMMON_C_FLAGS "-pipe -Wall -Werror")
set(COMMON_CXX_FLAGS "-pipe -Wall -Werror -Wc++20-extensions")

# Debug: No optimization, full debug info
set(CMAKE_C_FLAGS_DEBUG "${COMMON_C_FLAGS} -g3 -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${COMMON_CXX_FLAGS} -g3 -O0")

# Release: Optimize for size (-Os), minimal debug info
set(CMAKE_C_FLAGS_RELEASE "${COMMON_C_FLAGS} -Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${COMMON_CXX_FLAGS} -Os -DNDEBUG")

if(APPLE)
  # Issue with homebrew libraries, totally safe to add this flag.
  add_link_options("-Wl,-no_warn_duplicate_libraries")
endif()

# ##############################################################################
# Code Coverage Support
# ##############################################################################
# Only add code coverage when CMAKE_GCOV is True
if(CMAKE_GCOV)
  message(STATUS "Adding GCOV flags")
  # Add gcov support to all build types (typically used with Debug)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")

  add_custom_target(
    clean-gcov
    COMMAND find ${PROJECT_BINARY_DIR} -name \\*.gcda -exec rm {} \\\\;
    COMMAND find ${PROJECT_BINARY_DIR} -name \\*.gcno -exec rm {} \\\\;
    COMMENT "Cleaning GCOV statistics")
endif()

# ##############################################################################
# Platform Specific Settings
# ##############################################################################
# assume built-in pthreads on MacOS when building with gcc
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_THREAD_LIBS_INIT "-lpthread")
  set(CMAKE_HAVE_THREADS_LIBRARY 1)
  set(CMAKE_USE_WIN32_THREADS_INIT 0)
  set(CMAKE_USE_PTHREADS_INIT 1)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()
