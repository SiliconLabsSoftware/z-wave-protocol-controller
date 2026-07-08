configure_file(interface/inc/command_class_indicator_attributes.hpp.in
  ${CMAKE_CURRENT_LIST_DIR}/interface/generated/inc/command_class_indicator_attributes.hpp)

add_library(
  command_class_indicator STATIC
  src/command_class_indicator.cpp
  src/command_class_indicator_attribute_store.cpp
  src/command_class_indicator_mqtt.cpp
  generated/src/command_class_indicator_core.cpp
)

target_include_directories(command_class_indicator 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/interface/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_indicator
  PUBLIC
    command_class_common
    command_class_indicator_interface
)

add_subdirectory(interface)