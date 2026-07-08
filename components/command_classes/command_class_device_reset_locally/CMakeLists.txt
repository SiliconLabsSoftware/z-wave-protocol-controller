add_library(
  command_class_device_reset_locally STATIC
  src/command_class_device_reset_locally.cpp
  src/command_class_device_reset_locally_attribute_store.cpp
  src/command_class_device_reset_locally_mqtt.cpp
  generated/src/command_class_device_reset_locally_core.cpp
)

target_include_directories(command_class_device_reset_locally 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_device_reset_locally
  PUBLIC
    command_class_common
    command_class_device_reset_locally_interface
    command_class_association_grp_info_interface
    timer
)

add_subdirectory(interface)