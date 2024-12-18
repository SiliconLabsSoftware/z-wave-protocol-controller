include(FetchContent)

if(NOT DEFINED UNIFYSDK_GIT_REPOSITORY)
  set(UNIFYSDK_GIT_REPOSITORY https://github.com/SiliconLabs/UnifySDK)
endif()

# TODO: Pin stable revision once supported
if(NOT DEFINED UNIFYSDK_GIT_TAG)
  set(UNIFYSDK_GIT_TAG main)
endif()

FetchContent_Declare(
  UnifySDK
  GIT_REPOSITORY ${UNIFYSDK_GIT_REPOSITORY}
  GIT_TAG        ${UNIFYSDK_GIT_TAG}
  GIT_SUBMODULES_RECURSE True
  GIT_SHALLOW 1
)

# Pull only once, this has be refreshed by developer
if(NOT EXISTS ${CMAKE_BINARY_DIR}/_deps/unifysdk-src)
  message(STATUS "Fetching UnifySDK from git repository")
  FetchContent_MakeAvailable(UnifySDK)
else()
  message(STATUS "Using UnifySDK from previous fetch (may be outdated)")
  add_subdirectory(${CMAKE_BINARY_DIR}/_deps/unifysdk-src)
endif()
