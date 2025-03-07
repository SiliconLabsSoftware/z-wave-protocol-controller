# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-License-Identifier: Zlib
#
# This recipe allows to download CMock
# It can be used by projects which are depending on it
# Feel free to copy this (up to date) file everywhere it is needed

include(FetchContent)

if(NOT DEFINED CMOCK_GIT_REPOSITORY)
  if(DEFINED ENV{CMOCK_GIT_REPOSITORY})
    set(CMOCK_GIT_REPOSITORY $ENV{CMOCK_GIT_REPOSITORY})
  endif()
endif()
if("${CMOCK_GIT_REPOSITORY}" STREQUAL "")
  set(CMOCK_GIT_REPOSITORY "https://github.com/ThrowTheSwitch/CMock")
endif()

if(NOT DEFINED CMOCK_GIT_TAG)
  if(DEFINED ENV{CMOCK_GIT_TAG})
    set(CMOCK_GIT_TAG $ENV{CMOCK_GIT_TAG})
  else()
    set(CMOCK_GIT_TAG "v2.5.3")
  endif()
endif()
  
FetchContent_Declare(
  CMock
  GIT_REPOSITORY ${CMOCK_GIT_REPOSITORY}
  GIT_TAG        ${CMOCK_GIT_TAG}
  GIT_SUBMODULES_RECURSE True
  GIT_SHALLOW 1
)

message(STATUS "${CMAKE_PROJECT_NAME}: Depends: ${CMOCK_GIT_REPOSITORY}#${CMOCK_GIT_TAG}")
string(REGEX MATCH ".*/?main/?.*" CMOCK_UNSTABLE_GIT_TAG "${CMOCK_GIT_TAG}")
if(CMOCK_GIT_TAG STREQUAL "" OR CMOCK_UNSTABLE_GIT_TAG)
  message(WARNING "${CMAKE_PROJECT_NAME}: Declare CMOCK_GIT_TAG to stable version not: ${CMOCK_UNSTABLE_GIT_TAG}")
endif()

set(FETCHCONTENT_QUIET FALSE)
FetchContent_MakeAvailable(CMock)

message(STATUS "CMock Sources: ${cmock_SOURCE_DIR}")
message(STATUS "CMock Binaries: ${cmock_BINARY_DIR}")
