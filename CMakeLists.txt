cmake_minimum_required(VERSION 3.21)
project(unify-z-wave)

# ##############################################################################
# Include Configurations
# ##############################################################################

include(cmake/include/version.cmake)
include(cmake/include/build_permutations.cmake)

include(cmake/modules/FindUnifySDK.cmake)
if(NOT DEFINED COMMON_LOCATION)
  set(COMMON_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/_deps/unifysdk-src")
endif()

if(BUILD_TESTING)
  include(${CMAKE_SOURCE_DIR}/components/testframework/target_add_unittest.cmake)
  include(cmake/include/unittest.cmake)
endif()

# ##############################################################################
# Global includes
# ##############################################################################
include_directories(include ${CMAKE_CURRENT_BINARY_DIR}/include)

# ##############################################################################
# Subdirectories
# #############################################################################
add_subdirectory(applications)
include(cmake/include/package.cmake)
include(cmake/include/version_file.cmake)
