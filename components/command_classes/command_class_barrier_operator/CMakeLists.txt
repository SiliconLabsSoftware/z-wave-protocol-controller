add_library(
  command_class_barrier_operator STATIC
  src/command_class_barrier_operator.cpp
  src/command_class_barrier_operator_attribute_store.cpp
  src/command_class_barrier_operator_mqtt.cpp
  generated/src/command_class_barrier_operator_core.cpp
)

target_include_directories(command_class_barrier_operator 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_barrier_operator
  PUBLIC
    command_class_common
    command_class_barrier_operator_interface
)

add_subdirectory(interface)

if(UNIT_TEST)
  add_subdirectory(test)
endif()
