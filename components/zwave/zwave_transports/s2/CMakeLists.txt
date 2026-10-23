set(ZWAVE_INCLUDE_DIR
    ${ZPC_SOURCE_DIR}/../../components/zwave/zwave_definitions/include)

# Z-Wave S2 Nonce management "sub-library"
add_library(zwave_s2_nonce_management src/zwave_s2_nonce_management.c)

target_include_directories(zwave_s2_nonce_management PUBLIC include)

target_link_libraries(
  zwave_s2_nonce_management
  PUBLIC s2_controller
  PRIVATE zwave_controller)
target_compile_definitions(zwave_s2_nonce_management PRIVATE ZW_CONTROLLER)

set(KEYSTORE_SOURCES "src/zwave_s2_keystore.c")

add_library(zwave_s2_key_derivation STATIC src/zwave_s2_key_derivation.c)
target_include_directories(zwave_s2_key_derivation PUBLIC include)
target_link_libraries(zwave_s2_key_derivation PUBLIC crypto_core)
set_target_properties(zwave_s2_key_derivation PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 99)
set_source_files_properties(src/zwave_s2_key_derivation.c PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)

# NextNonce combines S2 nonce derivation with the generic CTR-DRBG.
add_library(zwave_s2_nextnonce STATIC src/nextnonce.c)
target_include_directories(zwave_s2_nextnonce PUBLIC include)
target_link_libraries(zwave_s2_nextnonce PUBLIC crypto_ctr_drbg)
set_target_properties(zwave_s2_nextnonce PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 99)

# Z-Wave S2 main library
add_library(
  zwave_s2
  src/zwave_s2_fixt.c ${KEYSTORE_SOURCES} src/zwave_s2_network.c
  src/zwave_s2_printf.c src/zwave_s2_process.c src/zwave_s2_transport.c)

set(DEFAULT_ZW-LIBS2_PATH libs/zw-libs2)
if(EXISTS ${ZW-LIBS2_LOCATION})
  set(ZW-LIBS2_PATH ${ZW-LIBS2_LOCATION})
  message(STATUS "Found ZW-LIBS2: ${ZW-LIBS2_LOCATION}")
  add_subdirectory(${ZW-LIBS2_PATH} ${CMAKE_CURRENT_BINARY_DIR}/zw-libs2)
else()
  set(ZW-LIBS2_PATH ${DEFAULT_ZW-LIBS2_PATH})
  add_subdirectory(${ZW-LIBS2_PATH})
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
  crypto
  zwave_s2_nextnonce
  zwave_s2_key_derivation)

target_include_directories(
  s2_controller
  PUBLIC
  include
  PRIVATE
  ${ZPC_SOURCE_DIR}/../../components/zwave/zwave_definitions/include
  ${ZW-LIBS2_PATH}/inclusion
  src)
# libs2 headers (S2.h, s2_keystore.h, ...) are consumed by targets that link
# s2_controller, so they must be exported.
target_include_directories(s2_controller PUBLIC ${ZW-LIBS2_PATH}/include)

target_link_libraries(
  zwave_s2
  PUBLIC zwave_s2_nonce_management
  PRIVATE s2_controller crypto zwave_controller zwave_s0
          zpc_attribute_store)

target_compile_definitions(zwave_s2 PRIVATE ZIPGW ZW_CONTROLLER)

if(UNIT_TEST)
  add_subdirectory(test)
endif()
