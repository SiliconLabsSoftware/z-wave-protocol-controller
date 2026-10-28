add_library(
  command_class_schedule STATIC
  src/command_class_schedule.cpp
  src/command_class_schedule_attribute_store.cpp
  src/command_class_schedule_mqtt.cpp
  generated/src/command_class_schedule_core.cpp
)

target_include_directories(command_class_schedule 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_schedule
  PUBLIC
    command_class_common
    command_class_schedule_interface
)

add_subdirectory(interface)

if(UNIT_TEST)
  add_subdirectory(test)
endif()
