cmake_minimum_required(VERSION 3.21)
project(z-wave-protocol-controller)

# ##############################################################################
# Include Configurations
# ##############################################################################

include(cmake/include/version.cmake)
include(cmake/include/build_permutations.cmake)

include(cmake/modules/FindUnifySDK.cmake)
if(NOT DEFINED COMMON_LOCATION)
  set(COMMON_LOCATION "${unifysdk_SOURCE_DIR}")
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
