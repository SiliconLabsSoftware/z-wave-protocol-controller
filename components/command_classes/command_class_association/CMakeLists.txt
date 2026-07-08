add_library(
  command_class_association STATIC
  src/command_class_association.cpp
  src/command_class_association_attribute_store.cpp
  src/command_class_association_mqtt.cpp
  generated/src/command_class_association_core.cpp
)

target_include_directories(command_class_association 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_association
  PUBLIC
    command_class_common
    command_class_association_interface
    command_class_association_grp_info_interface
)

add_subdirectory(interface)