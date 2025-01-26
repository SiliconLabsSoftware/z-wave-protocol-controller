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
if(NOT DEFINED UNIFY_SDK_BINARY_DIR)
  set(UNIFY_SDK_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/build/_deps/unifysdk-src")
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

# ##############################################################################
# Extra targets
# ##############################################################################

add_custom_target(unify-binary-dir
  DEPENDS unify
  COMMAND echo "Trick for building rust components depending on core libunify.so ${UNIFY_SDK_BINARY_DIR}"
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/components/
  COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${UNIFY_SDK_BINARY_DIR}/components/$<TARGET_FILE_NAME:unify>
    ${CMAKE_BINARY_DIR}/components/$<TARGET_FILE_NAME:unify>
)

include(cmake/include/package.cmake)
include(cmake/include/version_file.cmake)

if(BUILD_IMAGE_PROVIDER)
  # message(STATUS "Force to build uic-i-p package in zpc project (until it is built in core)")
  add_dependencies(uic_image_provider_build unify-binary-dir)
  add_dependencies(uic-image-provider_deb uic-image-provider)
  add_dependencies(uic-image-provider_deb package)
  add_dependencies(package_archive uic-image-provider_deb)
endif()

