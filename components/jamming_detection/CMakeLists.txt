find_package(nlohmann_json REQUIRED)

add_library(jamming_detection src/jamming_detection.cpp)

target_link_libraries(
  jamming_detection
  PUBLIC  mqtt_api threading
  PRIVATE zwave_api utils log sl_status_strings_lib nlohmann_json::nlohmann_json
)

target_include_directories(
  jamming_detection
  PUBLIC  include
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)

if(UNIT_TEST)
  add_subdirectory(test)
endif()
