if(NOT ZPC_SOURCE_DIR)
  set(ZPC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT ZPC_SOURCE_DIR)

project(zpc)
cmake_minimum_required(VERSION 3.19)

set(COMMON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../..)

set(CMAKE_MODULE_PATH ${COMMON_PATH}/cmake/modules ${CMAKE_MODULE_PATH})
set(CMAKE_INCLUDE_PATH ${COMMON_PATH}/cmake/include ${CMAKE_INCLUDE_PATH})

include(${COMMON_PATH}/cmake/include/target_interface_libraries.cmake)
include(${COMMON_PATH}/cmake/include/zpc_helper.cmake)
include(${COMMON_PATH}/cmake/include/compatible_platform.cmake)
include(${COMMON_PATH}/cmake/include/compiler_options.cmake)
include(${COMMON_PATH}/cmake/include/version.cmake)
include(${COMMON_PATH}/cmake/include/version_file.cmake)

include_directories(
  include
  ${COMMON_PATH}/components/platform/log/include
  ${COMMON_PATH}/components/config/include
  ${COMMON_PATH}/components/datastore/include
  ${COMMON_PATH}/components/attribute_store/include/# attribute_store.h
  ${COMMON_PATH}/components/smartstart/include
  ${COMMON_PATH}/components/mqtt/include # zpc_mqtt.hpp, mqtt_handler.hpp
  ${COMMON_PATH}/components/discovery/include # discovery_mqtt_api_initializer.hpp
  ${COMMON_PATH}/components/security/include # security_keys_dump_mqtt_api.hpp
  ${COMMON_PATH}/components/jamming_detection/include # jamming_detection.hpp
  ${COMMON_PATH}/components/ota/inc # update_manager.hpp
  ${COMMON_PATH}/components/main/include # init_builder.hpp
  ${CMAKE_CURRENT_BINARY_DIR}/include#sl_status_strings.h
  ${COMMON_PATH}/components/attribute_resolver/include/# attribute_resolver.h
  ${COMMON_PATH}/components/attribute_timeouts/include # attribute_timeouts.h
  ${COMMON_PATH}/components/zwave/zwave_network_management/src # zwave_network_management_handler.hpp
)

add_subdirectory(../../components components)

# ZPC executable
add_executable(zpc main.cpp)
target_link_libraries(
  zpc
  PUBLIC 
         network_monitor

         network_manager
         zpc_attribute_resolver
         zpc_attribute_store
         zpc_datastore
         utils
         zwave_controller
         zwave_definitions
         zwave_network_management
         zwave_rx
         zwave_transports
         zwave_tx
         zwave_tx_groups
         zwave_tx_scheme_selector

         zwave_api
         zwave_security_validation
         zwave_s2
         zwave_smartstart_management
         discovery
         security
         jamming_detection
         command_classes
         ota_update_manager
         zwave_command_class_manager

         component_connector
         system
         main
         attribute_timeouts
         zpc_attribute_resolver_core
         mqtt
         zpc_attribute_store_core
         datastore
         config
         log)

target_compile_definitions(
  zpc PRIVATE CMAKE_PROJECT_VERSION="${CMAKE_PROJECT_VERSION}")

target_include_directories(zpc PUBLIC ../../components/zwave/zwave_transports)

install(TARGETS zpc RUNTIME DESTINATION "bin" COMPONENT zpc)

if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
  install(
    DIRECTORY
    DESTINATION /var/lib/zpc
    COMPONENT zpc)
endif()
