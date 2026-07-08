add_library(
  command_class_transport_service STATIC
  src/command_class_transport_service.cpp
  src/command_class_transport_service_attribute_store.cpp
  src/command_class_transport_service_core.cpp
)

target_include_directories(command_class_transport_service 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_transport_service
  PUBLIC
    command_class_common
    command_class_transport_service_interface
)

add_subdirectory(interface)