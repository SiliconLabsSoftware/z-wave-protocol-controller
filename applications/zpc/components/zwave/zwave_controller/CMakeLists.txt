# Library
set(COMMON_SOURCES
    src/zwave_controller.c src/zwave_controller_callbacks.c
    src/zwave_controller_crc16.c src/zwave_controller_keyset.c
    src/zwave_controller_transport.c src/zwave_controller_utils.c
    src/zwave_controller_storage.c)

add_library(
  zwave_controller
  ${COMMON_SOURCES})

target_include_directories(zwave_controller PUBLIC include src)
target_link_libraries(
  zwave_controller
  PUBLIC zwave_rx zwave_tx zwave_network_management zwave_tx_scheme_selector
         zwave_definitions zwave_tx_groups zpc_config
  PRIVATE unify zwave_s0 zwave_s2 zpc_utils)
install(TARGETS zwave_controller LIBRARY DESTINATION lib)

if(BUILD_TESTING)
  target_add_mock(zwave_controller)

  target_include_directories(zwave_controller_mock PUBLIC src)

  add_subdirectory(test)
endif()


# zwave_controller_testlib library
add_library(
  zwave_controller_testlib
  ${COMMON_SOURCES})

target_include_directories(zwave_controller_testlib PUBLIC include src)
target_link_libraries(
  zwave_controller_testlib
  PUBLIC zwave_rx_testlib zwave_tx zwave_network_management_testlib zwave_tx_scheme_selector
          zwave_definitions zwave_tx_groups
  PRIVATE unify_testlib zwave_s0 zwave_s2_testlib zpc_utils)
install(TARGETS zwave_controller_testlib LIBRARY DESTINATION lib)

target_compile_definitions(
  zwave_controller_testlib
  PRIVATE ZWAVE_TESTLIB)