# Attribute transitions
add_library(attribute_timeouts OBJECT src/attribute_timeouts.cpp)

target_include_directories(
  attribute_timeouts
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  PRIVATE src)

target_link_libraries(
  attribute_timeouts
  PUBLIC zpc_attribute_store_core
  PRIVATE timer)
