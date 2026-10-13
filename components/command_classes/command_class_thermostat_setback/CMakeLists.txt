add_library(
  command_class_thermostat_setback STATIC
  src/command_class_thermostat_setback.cpp
  src/command_class_thermostat_setback_attribute_store.cpp
  src/command_class_thermostat_setback_mqtt.cpp
  generated/src/command_class_thermostat_setback_core.cpp
)

target_include_directories(command_class_thermostat_setback 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_thermostat_setback
  PUBLIC
    command_class_common
    command_class_thermostat_setback_interface
)

add_subdirectory(interface)