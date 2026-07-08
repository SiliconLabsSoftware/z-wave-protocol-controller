add_library(
  command_class_wake_up STATIC
  src/command_class_wake_up.cpp
  src/command_class_wake_up_attribute_store.cpp
  src/command_class_wake_up_mqtt.cpp
  generated/src/command_class_wake_up_core.cpp
)

target_include_directories(command_class_wake_up 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_wake_up
  PUBLIC
    command_class_common
    command_class_wake_up_interface
  PRIVATE
    attribute_timeouts
)

add_subdirectory(interface)