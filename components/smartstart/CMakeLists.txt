find_path(nlohmann_json_include nlohmann/json.hpp REQUIRED)
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

add_library(zwave_smartstart_management src/smartstart.cpp src/smartstart_mqtt_api.cpp)

target_include_directories(
  zwave_smartstart_management
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
         $<BUILD_INTERFACE:${nlohmann_json_include}>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

target_link_libraries(
  zwave_smartstart_management
  PUBLIC zwave_controller zwave_network_management zpc_attribute_store
  PRIVATE mqtt log s2_controller config timer mqtt_api network_monitor fmt::fmt)
