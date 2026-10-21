find_package(nlohmann_json REQUIRED)

add_library(discovery
  src/discovery_mqtt_api.cpp)

target_include_directories(
  discovery
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

target_link_libraries(
  discovery
  PUBLIC mqtt_api
  PRIVATE
    zwave_definitions
    zwave_network_management
    zwave_controller
    log
    nlohmann_json::nlohmann_json
    fmt::fmt)
