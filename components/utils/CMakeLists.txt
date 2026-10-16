# Find nlohmann_json, unfortunately the find_package doesn't work for this when
# cross compiling, thus we use the simplified method find_path instead
find_path(nlohmann_json_include nlohmann/json.hpp REQUIRED)

add_library(utils src/utils.cpp src/zwave_command_class_mqtt_utils.cpp)

target_include_directories(utils PUBLIC include ${nlohmann_json_include})

target_link_libraries(
  utils
  PUBLIC
  common
  log
  zwave_definitions
  PRIVATE
  fmt::fmt
)

install(TARGETS utils LIBRARY DESTINATION lib)
