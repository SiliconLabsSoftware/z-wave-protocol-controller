add_library(
  command_class_manufacturer_specific STATIC
  src/command_class_manufacturer_specific.cpp
  src/command_class_manufacturer_specific_attribute_store.cpp
  src/command_class_manufacturer_specific_mqtt.cpp
  generated/src/command_class_manufacturer_specific_core.cpp
)

target_include_directories(command_class_manufacturer_specific 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_manufacturer_specific
  PUBLIC
    command_class_common
    command_class_manufacturer_specific_interface
)

add_subdirectory(interface)