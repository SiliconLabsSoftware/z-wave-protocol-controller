set (COMMON_SOURCES
    src/nm_state_machine.c
    src/state_logging.c
    src/zwave_network_management.c
    src/zwave_network_management_fixt.c
    src/zwave_network_management_process.c
    src/zwave_network_management_helpers.c
    src/zwave_network_management_callbacks.c
    src/zwave_network_management_return_route_queue.c)

# Library
add_library(
  zwave_network_management
  ${COMMON_SOURCES})

target_link_libraries(zwave_network_management
                      PUBLIC unify
                      PRIVATE zwave_s0 zwave_s2 zwave_smartstart_management zwave_rx)
target_include_directories(zwave_network_management PUBLIC include)
install(TARGETS zwave_network_management LIBRARY DESTINATION lib)

if(BUILD_TESTING)
  # Unit tests
  add_subdirectory(test)

  ## Mocks
  target_add_mock(zwave_network_management)

  # Helper mocks
  add_mock(zwave_network_management_helpers_mock src/zwave_network_management_helpers.h)
  target_link_libraries(zwave_network_management_helpers_mock
                        PUBLIC zwave_controller)
endif()

# zwave_network_management_testlib library
add_library(
  zwave_network_management_testlib
  ${COMMON_SOURCES})

target_link_libraries(zwave_network_management_testlib
                      PUBLIC unify_testlib
                      PRIVATE zwave_s0 zwave_s2_testlib zwave_controller_testlib)
target_include_directories(zwave_network_management_testlib PUBLIC include)
install(TARGETS zwave_network_management_testlib LIBRARY DESTINATION lib)

target_compile_definitions(
  zwave_network_management_testlib
  PRIVATE ZWAVE_TESTLIB)
