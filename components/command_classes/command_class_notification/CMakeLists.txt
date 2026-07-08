configure_file(inc/command_class_notification_attributes.hpp.in
  ${CMAKE_CURRENT_LIST_DIR}/generated/inc/command_class_notification_attributes.hpp)

add_library(
  command_class_notification STATIC
  src/command_class_notification.cpp
  src/command_class_notification_attribute_store.cpp
  src/command_class_notification_mqtt.cpp
  generated/src/command_class_notification_core.cpp
)

target_include_directories(command_class_notification 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_notification
  PUBLIC
    command_class_common
    command_class_notification_interface
    command_class_association_grp_info_interface
    command_class_association_interface
)

add_subdirectory(interface)