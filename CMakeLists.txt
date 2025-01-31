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
  # https://gitlab.kitware.com/cmake/cmake/-/issues/22813#note_1620373
  option(BUILD_TESTING_PROPERTY_DISABLED "WARNING: Bypass some tests" True)
  if(BUILD_TESTING_PROPERTY_DISABLED)
    message(WARNING "TODO: Some tests are bypassed, please help fixing them")
  endif()
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
