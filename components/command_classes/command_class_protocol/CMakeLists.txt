add_library(
  command_class_protocol STATIC
  src/command_class_protocol.cpp
  src/command_class_protocol_attribute_store.cpp
  src/command_class_protocol_core.cpp
)

target_include_directories(command_class_protocol 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_protocol
  PUBLIC
    command_class_common
    command_class_protocol_interface
    command_class_supervision
    zwave_controller
)

add_subdirectory(interface)