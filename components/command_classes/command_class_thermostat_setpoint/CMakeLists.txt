add_library(
  command_class_thermostat_setpoint STATIC
  src/command_class_thermostat_setpoint.cpp
  src/command_class_thermostat_setpoint_attribute_store.cpp
  src/command_class_thermostat_setpoint_mqtt.cpp
  generated/src/command_class_thermostat_setpoint_core.cpp
)

target_include_directories(command_class_thermostat_setpoint 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_thermostat_setpoint
  PUBLIC
    command_class_common
    command_class_thermostat_setpoint_interface
)

add_subdirectory(interface)