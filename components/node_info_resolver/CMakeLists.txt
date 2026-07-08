add_library(
  node_info_resolver STATIC
  src/node_info_resolver.cpp
)

target_include_directories(node_info_resolver 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc/generated>
)

target_link_libraries(node_info_resolver
  PUBLIC
  zpc_attribute_store
  zpc_attribute_resolver
  zwave_definitions
  component_connector
  command_class_s2
  command_class_s0
  fmt::fmt)

configure_file(inc/node_info_resolver_events.hpp.in 
              ${CMAKE_CURRENT_LIST_DIR}/inc/generated/node_info_resolver_events.hpp)