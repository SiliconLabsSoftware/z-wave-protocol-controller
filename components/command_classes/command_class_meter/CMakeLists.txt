add_library(
  command_class_meter STATIC
  src/command_class_meter.cpp
  src/command_class_meter_attribute_store.cpp
  src/command_class_meter_mqtt.cpp
  generated/src/command_class_meter_core.cpp
)

target_include_directories(command_class_meter 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_meter
  PUBLIC
    command_class_common
    command_class_meter_interface
)

add_subdirectory(interface)