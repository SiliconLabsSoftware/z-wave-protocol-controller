add_library(
  command_class_s2 STATIC
  src/command_class_s2.cpp
  src/command_class_s2_attribute_store.cpp
  src/command_class_s2_core.cpp
)

target_include_directories(command_class_s2 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_s2
  PUBLIC
    command_class_common
    command_class_s2_interface
    zpc_attribute_store
)

add_subdirectory(interface)