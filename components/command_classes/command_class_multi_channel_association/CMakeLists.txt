add_library(
  command_class_multi_channel_association STATIC
  src/command_class_multi_channel_association.cpp
  src/command_class_multi_channel_association_attribute_store.cpp
  src/command_class_multi_channel_association_mqtt.cpp
  generated/src/command_class_multi_channel_association_core.cpp
)

target_include_directories(command_class_multi_channel_association 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_multi_channel_association
  PUBLIC
    command_class_common
    command_class_multi_channel_association_interface
)

add_subdirectory(interface)