# Find nlohmann_json, unfortunately the find_package doesn't work for this when
# cross compiling, thus we use the simplified method find_path instead
find_path(nlohmann_json_include nlohmann/json.hpp REQUIRED)

add_library(discovery
  src/discovery_mqtt_api.cpp)

target_include_directories(
  discovery
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  PRIVATE ${nlohmann_json_include})

target_link_libraries(
  discovery
  PUBLIC mqtt_api
  PRIVATE
    zwave_definitions
    zwave_network_management
    zwave_controller
    log
    fmt::fmt)
