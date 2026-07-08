# Library
add_library(
  zwave_network_management
  src/nm_state_machine.c
  src/state_logging.c
  src/zwave_network_management.c
  src/zwave_network_management_fixt.c
  src/zwave_network_management_process.cpp
  src/zwave_network_management_helpers.c
  src/zwave_network_management_callbacks.c
  src/zwave_network_management_return_route_queue.c)

target_link_libraries(zwave_network_management
                      PRIVATE zwave_s0 zwave_s2 zwave_smartstart_management zwave_rx threading)
target_include_directories(zwave_network_management PUBLIC include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
install(TARGETS zwave_network_management LIBRARY DESTINATION lib)
