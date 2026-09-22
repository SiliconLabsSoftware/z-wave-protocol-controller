set(CROSS_SYSTEM_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
# Setting -DOPTIONAL_SYSTEM_INCLUDE=<optional_include_path> allows to specify
# addional include path(s) when building libs2.
include_directories(include ${ZWAVE_INCLUDE_DIR} ${CROSS_SYSTEM_INCLUDE}
                    ${OPTIONAL_SYSTEM_INCLUDE} ${ZWAVE_CLASSCMD_DIR})

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3")
if(UNIT_TEST)
  enable_testing()
  add_subdirectory(test)
  message(STATUS "Adding unit test for libs2")
else()
  set(DISABLE_MOCK true)
endif()

add_subdirectory(crypto)
add_subdirectory(protocol)
add_subdirectory(inclusion)
if(NOT DEFINED DISABLE_MOCK)
  add_subdirectory(include/mock)
  add_subdirectory(inclusion/mock)
  add_subdirectory(crypto/mock)
endif()
