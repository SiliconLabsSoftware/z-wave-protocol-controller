set(ZWAVE_INCLUDE_DIR
    ${ZPC_SOURCE_DIR}/components/zwave/zwave_definitions/include)

# Z-Wave S2 Nonce management "sub-library"
add_library(zwave_s2_nonce_management src/zwave_s2_nonce_management.c)

target_include_directories(zwave_s2_nonce_management PUBLIC include)

target_link_libraries(
  zwave_s2_nonce_management
  PUBLIC s2_controller
  PRIVATE zwave_controller)
target_compile_definitions(zwave_s2_nonce_management PRIVATE ZW_CONTROLLER)

set(KEYSTORE_SOURCES "src/zwave_s2_keystore.c")

set(COMMON_SOURCES
    src/zwave_s2_fixt.c ${KEYSTORE_SOURCES} src/zwave_s2_network.c
    src/zwave_s2_printf.c src/zwave_s2_process.c src/zwave_s2_transport.c)

# Z-Wave S2 main library
add_library(
  zwave_s2
  ${COMMON_SOURCES})

set(DEFAULT_ZW-LIBS2_PATH libs/zw-libs2)
if(EXISTS ${ZW-LIBS2_LOCATION})
  set(ZW-LIBS2_PATH ${ZW-LIBS2_LOCATION})
  message(STATUS "Found ZW-LIBS2: ${ZW-LIBS2_LOCATION}")
  add_subdirectory(${ZW-LIBS2_PATH} ${CMAKE_CURRENT_BINARY_DIR}/zw-libs2)
else()
  set(ZW-LIBS2_PATH ${DEFAULT_ZW-LIBS2_PATH})
  add_subdirectory(${ZW-LIBS2_PATH})
endif()

if(NOT BUILD_TESTING)
  set(SKIP_TESTING
      ON
      CACHE BOOL "Skip building tests for zw-libs2")
endif()

target_include_directories(
  zwave_s2
  PUBLIC include
  PRIVATE ${ZW-LIBS2_PATH}/include src)
install(TARGETS zwave_s2 LIBRARY DESTINATION lib)

# Z-Wave S2 inclusion library for controller
add_library(
  s2_controller
  ${ZW-LIBS2_PATH}/protocol/S2.c
  ${ZW-LIBS2_PATH}/inclusion/s2_inclusion.c
  src/zwave_s2_inclusion_controller.c)

set_target_properties(
  s2_controller
  PROPERTIES
    COMPILE_DEFINITIONS "ZW_CONTROLLER")

target_link_libraries(
  s2_controller
  s2crypto
  aes)

target_include_directories(
  s2_controller
  PUBLIC
  include
  PRIVATE
  ${ZPC_SOURCE_DIR}/components/zwave/zwave_definitions/include
  ${ZW-LIBS2_PATH}/include
  ${ZW-LIBS2_PATH}/inclusion
  src)

target_link_libraries(
  zwave_s2
  PUBLIC zwave_s2_nonce_management unify
  PRIVATE s2_controller zwave_controller zwave_s0 zpc_attribute_store zpc_utils)

target_compile_definitions(zwave_s2 PRIVATE ZIPGW ZW_CONTROLLER)

if(BUILD_TESTING)
  target_add_mock(zwave_s2)
  add_subdirectory(test)

  add_mock(libs2_mock ${ZW-LIBS2_PATH}/include/S2.h
           ${ZW-LIBS2_PATH}/include/s2_inclusion.h
           ${ZW-LIBS2_PATH}/include/s2_protocol.h)
  add_mock(libs2_external_mock ${ZW-LIBS2_PATH}/include/S2_external.h)
  target_interface_libraries(libs2_mock zwave_definitions s2crypto aes)
  target_interface_libraries(libs2_external_mock zwave_definitions s2crypto aes)
  target_compile_definitions(libs2_mock PUBLIC ZIPGW)
  target_compile_definitions(libs2_external_mock PUBLIC ZIPGW)
endif()

# ZWAVE_TESTLIB S2 Library

add_library(
  zwave_s2_testlib
  ${COMMON_SOURCES})

target_include_directories(
  zwave_s2_testlib
  PUBLIC include
  PRIVATE ${ZW-LIBS2_PATH}/include src)
  
install(TARGETS zwave_s2_testlib LIBRARY DESTINATION lib)

target_link_libraries(
  zwave_s2_testlib
  PUBLIC zwave_s2_nonce_management unify
  PRIVATE s2_controller zwave_controller zwave_s0)
  
target_compile_definitions(zwave_s2_testlib PRIVATE ZIPGW ZW_CONTROLLER ZWAVE_TESTLIB)
