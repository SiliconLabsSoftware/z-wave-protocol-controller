# ZPC Attribute resolver core
add_library(zpc_attribute_resolver_core OBJECT src/attribute_resolver_rule.cpp
                                          src/attribute_resolver.cpp
                                          )

target_include_directories(
  zpc_attribute_resolver_core
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>)
target_link_libraries(
  zpc_attribute_resolver_core
  PUBLIC zpc_attribute_store_core datastore
  PRIVATE timer)

# ZPC Attribute resolver
add_library(
  zpc_attribute_resolver
  src/zpc_attribute_resolver.c src/zpc_attribute_resolver_callbacks.cpp
  src/zpc_attribute_resolver_group.cpp src/zpc_attribute_resolver_send.c)

target_include_directories(
  zpc_attribute_resolver
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)

target_link_libraries(
  zpc_attribute_resolver
  PRIVATE
    zpc_attribute_resolver_core
    zpc_attribute_store
    zwave_tx_groups
    utils
    command_class_supervision
    attribute_timeouts
)

install(TARGETS zpc_attribute_resolver LIBRARY DESTINATION lib)
