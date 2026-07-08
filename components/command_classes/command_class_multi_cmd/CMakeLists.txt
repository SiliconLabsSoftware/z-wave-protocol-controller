add_library(
  command_class_multi_cmd STATIC
  src/command_class_multi_cmd.cpp
  src/command_class_multi_cmd_attribute_store.cpp
  src/command_class_multi_cmd_mqtt.cpp
  generated/src/command_class_multi_cmd_core.cpp
)

target_include_directories(command_class_multi_cmd 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_multi_cmd
  PUBLIC
    command_class_common
    command_class_multi_cmd_interface
)

add_subdirectory(interface)