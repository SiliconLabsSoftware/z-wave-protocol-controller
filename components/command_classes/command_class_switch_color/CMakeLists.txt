add_library(
  command_class_switch_color STATIC
  src/command_class_switch_color.cpp
  src/command_class_switch_color_attribute_store.cpp
  src/command_class_switch_color_mqtt.cpp
  generated/src/command_class_switch_color_core.cpp
)

target_include_directories(command_class_switch_color 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_switch_color
  PUBLIC
    command_class_common
    command_class_switch_color_interface
)

add_subdirectory(interface)