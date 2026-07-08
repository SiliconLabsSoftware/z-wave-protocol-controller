# Find nlohmann_json, unfortunately the find_package doesn't work for this when
# cross compiling, thus we use the simplified method find_path instead
find_path(nlohmann_json_include nlohmann/json.hpp REQUIRED)

add_library(
  mqtt_api
  src/mqtt_api_base.cpp)

target_link_libraries(
  mqtt_api 
  PUBLIC mqtt
  PRIVATE
    zwave_definitions
    zwave_network_management
    zwave_controller
    log
    network_manager
    zwave_smartstart_management
    main
    # We link with main and not directly with mqtt to be able
    # to mock this service in the tests
    fmt::fmt)

target_include_directories(
  mqtt_api
  PUBLIC include
  PRIVATE ${nlohmann_json_include}
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../network_manager/include>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../smartstart/include>)
