add_library(
  command_class_protocol_lr STATIC
  src/command_class_protocol_lr.cpp
  src/command_class_protocol_lr_attribute_store.cpp
  src/command_class_protocol_lr_core.cpp
)

target_include_directories(command_class_protocol_lr 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_protocol_lr
  PUBLIC
    command_class_common
)

add_subdirectory(interface)