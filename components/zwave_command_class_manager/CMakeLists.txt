# Library
set(CC_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/../command_classes/assets")
set(ZWAVE_COMMAND_TYPE_CPP ${CMAKE_CURRENT_BINARY_DIR}/zwave_command_type.cpp)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

message(NOTICE "-- Generating zwave_command_type.cpp")
execute_process(
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/extract_get.py
          ${CC_ASSETS}/ZWave_custom_cmd_classes.xml
          ${CC_ASSETS}/supported.csv
  OUTPUT_FILE ${ZWAVE_COMMAND_TYPE_CPP}
  RESULT_VARIABLE ZWAVE_COMMAND_TYPE_RESULT
  ERROR_VARIABLE ZWAVE_COMMAND_TYPE_ERROR)
if(NOT ZWAVE_COMMAND_TYPE_RESULT EQUAL 0)
  message(FATAL_ERROR "zwave_command_type.cpp generation failed: ${ZWAVE_COMMAND_TYPE_ERROR}")
endif()

add_library(zwave_command_class_manager
  src/zwave_command_class_manager.cpp
  ${ZWAVE_COMMAND_TYPE_CPP})

target_include_directories(
  zwave_command_class_manager
  PUBLIC include
  PRIVATE src)
target_link_libraries(
  zwave_command_class_manager
  PUBLIC 
    zwave_command_class_base_interface
    zwave_controller 
    zwave_security_validation)

# Keep application-wide registration separate from the manager implementation.
# Command classes call the manager, while this fixture registers all command
# classes, so putting both in one library creates a static-library cycle.
add_library(zwave_command_class_manager_fixt
  src/zwave_command_class_manager_fixt.cpp)
target_link_libraries(zwave_command_class_manager_fixt
  PUBLIC zwave_command_class_manager
  PRIVATE command_classes)

install(TARGETS zwave_command_class_manager zwave_command_class_manager_fixt
        LIBRARY DESTINATION lib)

