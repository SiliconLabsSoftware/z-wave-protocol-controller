add_library(
  command_class_zwave_cmd_class STATIC
  src/command_class_zwave_cmd_class.cpp
  src/command_class_zwave_cmd_class_attribute_store.cpp
  generated/src/command_class_zwave_cmd_class_core.cpp
)

target_include_directories(command_class_zwave_cmd_class 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_zwave_cmd_class
  PUBLIC
    command_class_common
    command_class_zwave_cmd_class_interface
    command_class_supervision
    zwave_controller
)

add_subdirectory(interface)