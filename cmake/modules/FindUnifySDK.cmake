# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-License-Identifier: Zlib
#
# Origin: https://github.com/SiliconLabs/UnifySDK/pull/51
#
# This recipe allows to download Unify Core
# It can be used by projects which are depending on it
# Feel free to copy this (up to date) file everywhere it is needed

include(FetchContent)

if(NOT DEFINED UNIFYSDK_GIT_REPOSITORY)
  if(DEFINED ENV{UNIFYSDK_GIT_REPOSITORY})
    set(UNIFYSDK_GIT_REPOSITORY $ENV{UNIFYSDK_GIT_REPOSITORY})
  endif()
endif()
if("${UNIFYSDK_GIT_REPOSITORY}" STREQUAL "")
  set(UNIFYSDK_GIT_REPOSITORY "https://github.com/SiliconLabs/UnifySDK")
endif()

if(NOT DEFINED UNIFYSDK_GIT_TAG)
  if(DEFINED ENV{UNIFYSDK_GIT_TAG})
    set(UNIFYSDK_GIT_TAG $ENV{UNIFYSDK_GIT_TAG})
  endif()
endif()
if("${UNIFYSDK_GIT_TAG}" STREQUAL "")
  set(UNIFYSDK_GIT_TAG "main") # Override CMake default ("master")
endif()

if(${GIT_EXECUTABLE})
else()
  set(GIT_EXECUTABLE git)
endif()

FetchContent_Declare(
  UnifySDK
  GIT_REPOSITORY ${UNIFYSDK_GIT_REPOSITORY}
  GIT_TAG        ${UNIFYSDK_GIT_TAG}
  GIT_SUBMODULES_RECURSE True
  GIT_SHALLOW 1

  # Prevent "fatal: unable to auto-detect email address"
  GIT_CONFIG user.email=nobody@UnifySDK.localhost

  PATCH_COMMAND ${GIT_EXECUTABLE}
      -C <SOURCE_DIR>
      am
      ${PROJECT_SOURCE_DIR}/patches/UnifySDK/0001-UIC-3202-Relax-compiler-warnings-to-support-more-com.patch
)

message(STATUS "${CMAKE_PROJECT_NAME}: Depends: ${UNIFYSDK_GIT_REPOSITORY}#${UNIFYSDK_GIT_TAG}")
string(REGEX MATCH ".*/?main/?.*" UNIFYSDK_UNSTABLE_GIT_TAG "${UNIFYSDK_GIT_TAG}")
if(UNIFYSDK_GIT_TAG STREQUAL "" OR UNIFYSDK_UNSTABLE_GIT_TAG)
  message(WARNING "${CMAKE_PROJECT_NAME}: Declare UNIFYSDK_GIT_TAG to stable version not: ${UNIFYSDK_UNSTABLE_GIT_TAG}")
endif()

set(FETCHCONTENT_QUIET FALSE)
FetchContent_MakeAvailable(UnifySDK)

# message(STATUS "UnifySDK Sources: ${unifysdk_SOURCE_DIR}")
# message(STATUS "UnifySDK Binaries: ${unifysdk_BINARY_DIR}")
if(BUILD_TESTING)
  option(unifysdk_BUILD_TESTING_PROPERTY_DISABLED "WARNING: UnifySDK: Bypass some tests" True)
  if(NOT unifysdk_BUILD_TESTING_PROPERTY_DISABLED)
    message(WARNING "UnifySDK tests may break, skip them with ctest")
  else()
    message(WARNING "UnifySDK tests, some are bypassed")
    if(NOT CMAKE_VERSION VERSION_LESS 3.7.28)
      message(WARNING "Consider to upgrade cmake for https://gitlab.kitware.com/cmake/cmake/-/issues/22813#note_1620373 ${CMAKE_VERSION}")
    else()
      set_tests_properties(unify_build
        DIRECTORY ${unifysdk_SOURCE_DIR}/components
        PROPERTIES DISABLED True
      )
      set_tests_properties(attribute_mapper_uam_test_example_1
        DIRECTORY ${unifysdk_SOURCE_DIR}/components/uic_attribute_mapper/test
        PROPERTIES DISABLED True
      )
      set_tests_properties(attribute_mapper_uam_test_example_2
        DIRECTORY ${unifysdk_SOURCE_DIR}/components/uic_attribute_mapper/test
        PROPERTIES DISABLED True
      )
    endif()
  endif()
endif()
