add_library(
  command_class_inclusion_controller STATIC
  src/command_class_inclusion_controller.cpp
  src/command_class_inclusion_controller_attribute_store.cpp
  generated/src/command_class_inclusion_controller_core.cpp
)

target_include_directories(command_class_inclusion_controller 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_inclusion_controller
  PUBLIC
    command_class_common
    command_class_inclusion_controller_interface
)

add_subdirectory(interface)