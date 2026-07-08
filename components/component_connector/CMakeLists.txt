add_library(
  component_connector STATIC
  src/component_connector.cpp
)

target_include_directories(component_connector 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc/generated>
)

configure_file(inc/component_connector_common_events.hpp.in 
              ${CMAKE_CURRENT_LIST_DIR}/inc/generated/component_connector_common_events.hpp)

# TODO: Thsi component should not have any zwave specific dependencies
target_link_libraries(component_connector
  PUBLIC
  zwave_definitions
  zwave_controller
  main
  threading
  fmt::fmt)