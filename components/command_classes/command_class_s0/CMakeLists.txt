add_library(
  command_class_s0 STATIC
  src/command_class_s0.cpp
  src/command_class_s0_attribute_store.cpp
  src/command_class_s0_core.cpp
)

target_include_directories(command_class_s0 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_s0
  PUBLIC
    command_class_common
    command_class_s0_interface
)

add_subdirectory(interface)