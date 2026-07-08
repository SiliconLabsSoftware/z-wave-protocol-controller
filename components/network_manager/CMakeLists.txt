if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
# Find nlohmann_json, unfortunately the find_package doesn't work for this when
# cross compiling, thus we use the simplified method find_path instead
find_path(nlohmann_json_include nlohmann/json.hpp REQUIRED)

# UCL MQTT library
add_library(
  network_manager
  src/zpc_mqtt_node_interview.c src/zpc_network_management.cpp
  src/zpc_nm_neighbor_discovery.cpp
  src/network_management_mqtt_api.cpp
)

target_link_libraries(
  network_manager
  PUBLIC zwave_definitions
  PRIVATE zpc_attribute_store timer mqtt threading mqtt_api component_connector device_interviewer fmt::fmt zwave_api zwave_controller)

target_include_directories(
  network_manager
  PUBLIC include
  PRIVATE ${nlohmann_json_include}
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)
