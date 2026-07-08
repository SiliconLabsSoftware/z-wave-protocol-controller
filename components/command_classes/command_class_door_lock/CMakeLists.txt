add_library(
  command_class_door_lock STATIC
  src/command_class_door_lock.cpp
  src/command_class_door_lock_attribute_store.cpp
  src/command_class_door_lock_mqtt.cpp
  generated/src/command_class_door_lock_core.cpp
)

target_include_directories(command_class_door_lock 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_door_lock
  PUBLIC
    command_class_common
    command_class_door_lock_interface
)

add_subdirectory(interface)