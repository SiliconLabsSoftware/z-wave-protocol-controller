add_library(
  command_class_switch_binary STATIC
  src/command_class_switch_binary.cpp
  src/command_class_switch_binary_attribute_store.cpp
  src/command_class_switch_binary_mqtt.cpp
  generated/src/command_class_switch_binary_core.cpp
)

target_include_directories(command_class_switch_binary 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_switch_binary
  PUBLIC
    command_class_common
    command_class_switch_binary_interface
)

add_subdirectory(interface)