add_library(
  command_class_configuration STATIC
  src/command_class_configuration.cpp
  src/command_class_configuration_attribute_store.cpp
  src/command_class_configuration_mqtt.cpp
  generated/src/command_class_configuration_core.cpp
)

target_include_directories(command_class_configuration 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_configuration
  PUBLIC
    command_class_common
    command_class_configuration_interface
)

add_subdirectory(interface)