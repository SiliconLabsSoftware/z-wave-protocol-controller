# Minimal Unity + mock helpers for zw-libs2 unit tests.
# Provides: add_unity_test(), targets unity, mock, and UNIT_TEST.
# Do not restore Unify target_add_unittest / CMock for the whole tree.

if(COMMAND ADD_UNITY_TEST)
  return()
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})

set(TEST_TOOLS_DIR
    ${CMAKE_CURRENT_LIST_DIR}/../unity
    CACHE INTERNAL "Directory containing gen_test_runner.py")

include(FetchContent)
FetchContent_Declare(
  throwtheswitch_unity
  GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
  GIT_TAG v2.6.1
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(throwtheswitch_unity)

if(NOT TARGET unity)
  add_library(unity STATIC ${throwtheswitch_unity_SOURCE_DIR}/src/unity.c)
  target_include_directories(unity PUBLIC ${throwtheswitch_unity_SOURCE_DIR}/src)
  target_compile_options(unity PRIVATE -fPIC)
endif()

if(NOT TARGET mock)
  add_library(mock ${CMAKE_CURRENT_LIST_DIR}/../unity/mock/mock_control.c)
  target_link_libraries(mock PUBLIC unity)
  target_compile_definitions(mock PUBLIC USE_UNITY)
  target_include_directories(mock PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../unity/mock)
endif()

if(NOT TARGET UNIT_TEST)
  add_custom_target(UNIT_TEST)
endif()

function(ADD_UNITY_TEST)
  set(OPTIONS "USE_CPP" "DISABLED")
  set(SINGLE_VALUE_ARGS "NAME" "TEST_BASE")
  set(MULTI_VALUE_ARGS "FILES" "LIBRARIES" "INCLUDES")
  cmake_parse_arguments(ADD_UNITY_TEST "${OPTIONS}" "${SINGLE_VALUE_ARGS}"
                        "${MULTI_VALUE_ARGS}" ${ARGN})

  set(RUNNER_EXTENSION c)
  if(ADD_UNITY_TEST_USE_CPP)
    set(RUNNER_EXTENSION cpp)
  endif()

  set(RUNNER_BASE "${ADD_UNITY_TEST_NAME}.${RUNNER_EXTENSION}")
  if(NOT "${ADD_UNITY_TEST_TEST_BASE}" STREQUAL "")
    set(RUNNER_BASE "${ADD_UNITY_TEST_TEST_BASE}")
  endif()

  if("${ADD_UNITY_TEST_NAME}" STREQUAL "")
    list(GET ADD_UNITY_TEST_UNPARSED_ARGUMENTS 0 ADD_UNITY_TEST_NAME)
    list(REMOVE_AT ADD_UNITY_TEST_UNPARSED_ARGUMENTS 0)
    set(RUNNER_BASE "${ADD_UNITY_TEST_NAME}.${RUNNER_EXTENSION}")
    set(ADD_UNITY_TEST_FILES "${ADD_UNITY_TEST_UNPARSED_ARGUMENTS}")
  endif()

  set(RUNNER_FILE
      ${CMAKE_CURRENT_BINARY_DIR}/${ADD_UNITY_TEST_NAME}_runner.${RUNNER_EXTENSION})

  add_custom_command(
    OUTPUT ${RUNNER_FILE}
    COMMAND
      ${PYTHON_EXECUTABLE} ${TEST_TOOLS_DIR}/gen_test_runner.py ${RUNNER_BASE} >
      ${RUNNER_FILE}
    DEPENDS ${RUNNER_BASE} ${TEST_TOOLS_DIR}/gen_test_runner.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

  add_executable(${ADD_UNITY_TEST_NAME} ${RUNNER_FILE} ${RUNNER_BASE}
                 ${ADD_UNITY_TEST_FILES})
  add_test(NAME ${ADD_UNITY_TEST_NAME} COMMAND ${ADD_UNITY_TEST_NAME})
  target_link_libraries(${ADD_UNITY_TEST_NAME} PRIVATE unity
                      ${ADD_UNITY_TEST_LIBRARIES})
  target_include_directories(${ADD_UNITY_TEST_NAME} PRIVATE .
                             ${ADD_UNITY_TEST_INCLUDES})

  if(ADD_UNITY_TEST_DISABLED)
    set_tests_properties(${ADD_UNITY_TEST_NAME} PROPERTIES DISABLED True)
  endif()

  add_dependencies(UNIT_TEST ${ADD_UNITY_TEST_NAME})
endfunction()
