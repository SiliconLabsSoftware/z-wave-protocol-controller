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
  include(cmake/modules/FindCMock.cmake)
  set(THS-CMOCK_LOCATION "${cmock_SOURCE_DIR}")
  set(THS-UNITY_LOCATION  "${cmock_SOURCE_DIR}/vendor/unity")
  # https://gitlab.kitware.com/cmake/cmake/-/issues/22813#note_1620373
  option(BUILD_TESTING_PROPERTY_DISABLED "WARNING: Bypass some tests" True)
  if(BUILD_TESTING_PROPERTY_DISABLED)
    message(WARNING "TODO: Some tests are bypassed, please help fixing them")
  endif()
  # Testframework deduplicated, only one to be used
  set(ZPC_TESTFRAMEWORK_PATH
    ${unifysdk_SOURCE_DIR}/components/testframework
  )
  include(${ZPC_TESTFRAMEWORK_PATH}/target_add_unittest.cmake)
  include(cmake/include/unittest.cmake)
endif()

include(cmake/include/doxygen.cmake)

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
  COMMAND echo "Trick for building rust components depending on core libunify.so ${unifysdk_BINARY_DIR}"
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/components/
  COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${unifysdk_BINARY_DIR}/components/$<TARGET_FILE_NAME:unify>
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
