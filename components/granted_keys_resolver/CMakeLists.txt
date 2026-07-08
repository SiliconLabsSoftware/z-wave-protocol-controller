add_library(
  granted_keys_resolver STATIC
  src/granted_keys_resolver.cpp
)

target_include_directories(granted_keys_resolver 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc/generated>
)

target_link_libraries(granted_keys_resolver
  PUBLIC
  zpc_attribute_store
  zpc_attribute_resolver
  zwave_definitions
  component_connector
  fmt::fmt)

configure_file(inc/granted_keys_resolver_events.hpp.in 
              ${CMAKE_CURRENT_LIST_DIR}/inc/generated/granted_keys_resolver_events.hpp)