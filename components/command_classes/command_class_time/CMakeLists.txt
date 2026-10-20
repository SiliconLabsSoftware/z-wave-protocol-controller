add_library(
  command_class_time STATIC
  src/command_class_time.cpp
  src/command_class_time_attribute_store.cpp
  src/command_class_time_mqtt.cpp
  generated/src/command_class_time_core.cpp
)

target_include_directories(command_class_time 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_time
  PUBLIC
    command_class_common
    command_class_time_interface
    timer
    zwave_tx_scheme_selector
)

add_subdirectory(interface)
