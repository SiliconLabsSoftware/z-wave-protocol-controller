add_library(
  system STATIC
  src/system_events.cpp
)

target_include_directories(system 
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

target_link_libraries(system
  PUBLIC
  component_connector
  zwave_definitions
  zwave_controller
  command_class_common
  log
  main
  PRIVATE
  zpc_attribute_store
)
