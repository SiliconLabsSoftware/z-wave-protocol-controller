add_library(
  command_class_version STATIC
  src/command_class_version.cpp
  src/command_class_version_attribute_store.cpp
  src/command_class_version_mqtt.cpp
  generated/src/command_class_version_core.cpp
)

target_include_directories(command_class_version 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_version
  PUBLIC
    command_class_common
    command_class_version_interface
)

add_subdirectory(interface)