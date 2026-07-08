if(NOT MQTT_CLIENT_LIBRARY)
  # Currently the default MQTT client-library is Paho MQTT C++
  set(MQTT_CLIENT_LIBRARY PahoMQTTCpp)
endif(NOT MQTT_CLIENT_LIBRARY)

# Library
if(MQTT_CLIENT_LIBRARY STREQUAL PahoMQTTCpp)
  find_package(PahoMQTTCpp REQUIRED)
  set(MQTT_LIBRARY ${PAHO_MQTT_CPP_LIBRARIES})
  set(MQTT_LIBRARY_INCLUDE_DIR ${PAHO_MQTT_CPP_INCLUDE_DIR})

  # elseif(MQTT_CLIENT_LIBRARY STREQUAL SomeOtherLibrary)
else()
  message(
    FATAL_ERROR "MQTT_CLIENT_LIBRARY '${MQTT_CLIENT_LIBRARY}' not implemented.")
endif()

# NOTE: This should be called only once for all components
#
# Find nlohmann_json, unfortunately the find_package doesn't work for this when
# cross compiling, thus we use the simplified method find_path instead
find_path(nlohmann_json_include nlohmann/json.hpp REQUIRED)

find_package(FMT)

# MQTT library
add_library(
  mqtt OBJECT
  src/mqtt_handler.cpp)

target_include_directories(
  mqtt
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  ${MQTT_LIBRARY_INCLUDE_DIR}
  ${PAHO_MQTT_C_INCLUDE_DIR})

target_link_libraries(mqtt PUBLIC ${MQTT_LIBRARY}
  config log datastore
  sl_status_strings_lib timer threading fmt::fmt)

# ZPC MQTT library
add_library(
  zpc_mqtt
  src/zpc_mqtt.cpp
  src/zpc_mqtt_utils.cpp

  src/private/zpc_mqtt_attribute_registration.cpp
  src/private/zpc_mqtt_command_registration.cpp
)

target_link_libraries(
  zpc_mqtt
  PRIVATE
  zwave_definitions
  zpc_attribute_store
  utils
  threading
  # We link with main and not directly with mqtt to be able
  # to mock this service in the tests 
  fmt::fmt)

target_include_directories(
  zpc_mqtt
  PUBLIC include
  PRIVATE
  ${nlohmann_json_include}
  ${MQTT_LIBRARY_INCLUDE_DIR}
  ${PAHO_MQTT_C_INCLUDE_DIR}
  src/private
  src)

