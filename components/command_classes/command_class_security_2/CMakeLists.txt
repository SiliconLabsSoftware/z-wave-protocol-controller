add_library(
  command_class_security_2 STATIC
  src/command_class_security_2.cpp
  src/command_class_security_2_attribute_store.cpp
  generated/src/command_class_security_2_core.cpp
)

target_include_directories(command_class_security_2 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_security_2
  PUBLIC
    command_class_common
    command_class_security_2_interface
)

add_subdirectory(interface)