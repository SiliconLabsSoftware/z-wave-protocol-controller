add_library(
  command_class_security STATIC
  src/command_class_security.cpp
  src/command_class_security_attribute_store.cpp
  generated/src/command_class_security_core.cpp
)

target_include_directories(command_class_security 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_security
  PUBLIC
    command_class_common
    command_class_security_interface
)

add_subdirectory(interface)