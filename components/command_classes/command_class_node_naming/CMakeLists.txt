add_library(
  command_class_node_naming STATIC
  src/command_class_node_naming.cpp
  src/command_class_node_naming_attribute_store.cpp
  src/command_class_node_naming_mqtt.cpp
  generated/src/command_class_node_naming_core.cpp
)

target_include_directories(command_class_node_naming 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_node_naming
  PUBLIC
    command_class_common
    command_class_node_naming_interface
)

add_subdirectory(interface)