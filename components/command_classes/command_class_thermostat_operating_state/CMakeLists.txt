add_library(
  command_class_thermostat_operating_state STATIC
  src/command_class_thermostat_operating_state.cpp
  src/command_class_thermostat_operating_state_attribute_store.cpp
  src/command_class_thermostat_operating_state_mqtt.cpp
  generated/src/command_class_thermostat_operating_state_core.cpp
)

target_include_directories(command_class_thermostat_operating_state 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_thermostat_operating_state
  PUBLIC
    command_class_common
    command_class_thermostat_operating_state_interface
)

add_subdirectory(interface)