add_library(
  command_class_basic STATIC
  src/command_class_basic.cpp
  src/command_class_basic_attribute_store.cpp
  src/command_class_basic_mqtt.cpp
  generated/src/command_class_basic_core.cpp
)

target_include_directories(command_class_basic 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_basic
  PUBLIC
    command_class_common
    command_class_basic_interface
)

add_subdirectory(interface)
