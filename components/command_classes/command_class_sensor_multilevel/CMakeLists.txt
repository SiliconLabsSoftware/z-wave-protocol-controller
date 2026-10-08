add_library(
  command_class_sensor_multilevel STATIC
  src/command_class_sensor_multilevel.cpp
  src/command_class_sensor_multilevel_attribute_store.cpp
  src/command_class_sensor_multilevel_mqtt.cpp
  generated/src/command_class_sensor_multilevel_core.cpp
)

target_include_directories(command_class_sensor_multilevel 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_sensor_multilevel
  PUBLIC
    command_class_common
    command_class_sensor_multilevel_interface
)

add_subdirectory(interface)