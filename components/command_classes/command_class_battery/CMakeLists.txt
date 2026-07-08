add_library(
  command_class_battery STATIC
  src/command_class_battery.cpp
  src/command_class_battery_attribute_store.cpp
  src/command_class_battery_mqtt.cpp
  generated/src/command_class_battery_core.cpp
)

target_include_directories(command_class_battery 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_battery
  PUBLIC
    command_class_common
    command_class_battery_interface
)

add_subdirectory(interface)