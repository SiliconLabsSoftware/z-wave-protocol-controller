add_library(
  command_class_powerlevel STATIC
  src/command_class_powerlevel.cpp
  src/command_class_powerlevel_attribute_store.cpp
  generated/src/command_class_powerlevel_core.cpp
)

target_include_directories(command_class_powerlevel 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_powerlevel
  PUBLIC
    timer
    command_class_common
    command_class_powerlevel_interface
)

add_subdirectory(interface)