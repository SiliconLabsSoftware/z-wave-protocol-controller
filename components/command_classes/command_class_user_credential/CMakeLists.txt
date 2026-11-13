add_library(
  command_class_user_credential STATIC
  src/command_class_user_credential.cpp
  src/command_class_user_credential_rules.cpp
  src/command_class_user_credential_attribute_store.cpp
  src/command_class_user_credential_mqtt.cpp
  generated/src/command_class_user_credential_core.cpp
)

target_include_directories(command_class_user_credential 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_user_credential
  PUBLIC
    command_class_common
    command_class_user_credential_interface
)

add_subdirectory(interface)

if(UNIT_TEST)
  add_subdirectory(test)
endif()