add_library(
  command_class_multi_channel STATIC
  src/command_class_multi_channel.cpp
  src/command_class_multi_channel_attribute_store.cpp
  src/command_class_multi_channel_mqtt.cpp
  generated/src/command_class_multi_channel_core.cpp
)

target_include_directories(command_class_multi_channel 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_multi_channel
  PUBLIC
    command_class_common
    command_class_multi_channel_interface
)

add_subdirectory(interface)