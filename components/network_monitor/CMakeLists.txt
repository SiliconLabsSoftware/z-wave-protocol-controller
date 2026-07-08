# Library
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
find_path(nlohmann_json_include nlohmann/json.hpp REQUIRED)
set(NETWORK_STATE_SOURCES
  src/network_monitor.cpp
  src/network_monitor_mqtt_api.cpp
  src/network_monitor_span_persistence.cpp
  src/keep_sleeping_nodes_alive.cpp
  src/failing_node_monitor.cpp
  src/network_monitor_utils.cpp)

configure_file(include/network_monitor_attribute_store.hpp.in 
  ${CMAKE_CURRENT_LIST_DIR}/include/generated/network_monitor_attribute_store.hpp)

add_library(network_monitor ${NETWORK_STATE_SOURCES})
target_include_directories(
  network_monitor
  PUBLIC include include/generated
  PRIVATE src ${nlohmann_json_include})
target_link_libraries(
  network_monitor
  INTERFACE zwave_definitions
  PUBLIC zpc_attribute_store network_manager threading mqtt_api
  PRIVATE zwave_s2 timer component_connector command_class_wake_up_interface attribute_timeouts command_class_version_interface)
