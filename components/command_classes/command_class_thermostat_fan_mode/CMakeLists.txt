add_library(
  command_class_thermostat_fan_mode STATIC
  src/command_class_thermostat_fan_mode.cpp
  src/command_class_thermostat_fan_mode_attribute_store.cpp
  src/command_class_thermostat_fan_mode_mqtt.cpp
  generated/src/command_class_thermostat_fan_mode_core.cpp
)

target_include_directories(command_class_thermostat_fan_mode 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_thermostat_fan_mode
  PUBLIC
    command_class_common
    command_class_thermostat_fan_mode_interface
)

add_subdirectory(interface)