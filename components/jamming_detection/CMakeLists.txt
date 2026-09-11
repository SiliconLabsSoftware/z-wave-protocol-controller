add_library(jamming_detection src/jamming_detection.cpp)

target_link_libraries(
  jamming_detection
  PUBLIC  mqtt_api
  PRIVATE zwave_api log
)

target_include_directories(
  jamming_detection
  PUBLIC  include
  PRIVATE ${nlohmann_json_include}
          $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)
