# GoogleTest / GoogleMock helpers for ZPC component unit tests.

if(COMMAND ADD_ZPC_GTEST)
  return()
endif()

include(FetchContent)

set(GTEST_REPOSITORY https://github.com/google/googletest.git)
set(GTEST_TAG v1.15.2)

set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY ${GTEST_REPOSITORY}
  GIT_TAG ${GTEST_TAG}
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(googletest)

# ZPC uses -Werror globally; relax warnings for third-party googletest sources.
foreach(_zpc_gtest_dep gtest gtest_main gmock gmock_main)
  if(TARGET ${_zpc_gtest_dep})
    target_compile_options(${_zpc_gtest_dep} PRIVATE -Wno-error -Wno-character-conversion)
  endif()
endforeach()

include(GoogleTest)

if(NOT TARGET UNIT_TEST)
  add_custom_target(UNIT_TEST)
endif()

function(ADD_ZPC_GTEST)
  set(OPTIONS "")
  set(SINGLE_VALUE_ARGS "NAME")
  set(MULTI_VALUE_ARGS "SRCS" "LIBS" "INCLUDES")
  cmake_parse_arguments(ADD_ZPC_GTEST "${OPTIONS}" "${SINGLE_VALUE_ARGS}"
                        "${MULTI_VALUE_ARGS}" ${ARGN})

  if("${ADD_ZPC_GTEST_NAME}" STREQUAL "")
    message(FATAL_ERROR "add_zpc_gtest: NAME is required")
  endif()

  add_executable(${ADD_ZPC_GTEST_NAME} ${ADD_ZPC_GTEST_SRCS})
  target_link_libraries(
    ${ADD_ZPC_GTEST_NAME}
    PRIVATE GTest::gtest_main GTest::gmock ${ADD_ZPC_GTEST_LIBS})
  target_include_directories(${ADD_ZPC_GTEST_NAME} PRIVATE ${ADD_ZPC_GTEST_INCLUDES})

  gtest_discover_tests(${ADD_ZPC_GTEST_NAME})

  add_dependencies(UNIT_TEST ${ADD_ZPC_GTEST_NAME})
endfunction()
