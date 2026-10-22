find_package(nlohmann_json REQUIRED)

add_library(utils src/utils.cpp src/zwave_command_class_mqtt_utils.cpp)

target_include_directories(utils PUBLIC include)

target_link_libraries(
  utils
  PUBLIC
  common
  log
  zwave_definitions
  nlohmann_json::nlohmann_json
  PRIVATE
  fmt::fmt
)

install(TARGETS utils LIBRARY DESTINATION lib)
