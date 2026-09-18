add_library(
  command_class_zwave_long_range STATIC
  src/command_class_zwave_long_range.cpp
  src/command_class_zwave_long_range_attribute_store.cpp
  generated/src/command_class_zwave_long_range_core.cpp
)

target_include_directories(command_class_zwave_long_range 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_zwave_long_range
  PUBLIC
    command_class_common
    command_class_zwave_long_range_interface
)

add_subdirectory(interface)