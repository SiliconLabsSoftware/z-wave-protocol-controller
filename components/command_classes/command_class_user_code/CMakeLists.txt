add_library(
  command_class_user_code STATIC
  src/command_class_user_code.cpp
  src/command_class_user_code_attribute_store.cpp
  src/command_class_user_code_mqtt.cpp
  generated/src/command_class_user_code_core.cpp
)

target_include_directories(command_class_user_code 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_user_code
  PUBLIC
    command_class_common
    command_class_user_code_interface
)

add_subdirectory(interface)