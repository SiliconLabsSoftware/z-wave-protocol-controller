if(NOT TARGET s2crypto_zwave)
  add_subdirectory(crypto)
endif()

# OpenSSL libcrypto is already linked transitively by Paho-MQTT-C's TLS
# variant; using it here adds no new runtime dependency.
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

find_package(nlohmann_json REQUIRED)

add_library(security
  src/seal_x25519.cpp
  src/security_keys_dump_writer.cpp
  src/security_keys_dump_mqtt_api.cpp)

target_include_directories(
  security
  PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>)

target_link_libraries(
  security
  PUBLIC  mqtt_api
  PRIVATE OpenSSL::Crypto
          config
          log
          zwave_s2
          # The mqtt_api public header transitively pulls in headers from
          # zwave_definitions / zwave_network_management / zwave_controller.
          # mqtt_api links these PRIVATE, so each consumer must re-declare
          # them. This mirrors components/discovery's pattern exactly.
          zwave_definitions
          zwave_network_management
          zwave_controller
          fmt::fmt
          nlohmann_json::nlohmann_json)
