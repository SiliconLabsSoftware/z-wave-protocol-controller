add_library(
  command_class_supervision STATIC
  src/command_class_supervision.cpp
  src/command_class_supervision_attribute_store.cpp
  src/command_class_supervision_mqtt.cpp
  generated/src/command_class_supervision_core.cpp
  src/zwave_command_class_supervision.c
  src/zwave_command_class_supervision_process.cpp
)

target_include_directories(command_class_supervision 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_supervision
  PUBLIC
    command_class_common
    command_class_supervision_interface
    timer
)

add_subdirectory(interface)