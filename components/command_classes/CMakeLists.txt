message(NOTICE "-- Generating command classes")
execute_process(COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/command_class_generator/main.py
  --xml ${CMAKE_SOURCE_DIR}/scripts/command_class_generator/zwave.xml
  --xsd ${CMAKE_SOURCE_DIR}/scripts/command_class_generator/zwave.xsd
  --cfg ${CMAKE_SOURCE_DIR}/scripts/command_class_generator/config.yaml
  --templates ${CMAKE_SOURCE_DIR}/scripts/command_class_generator/templates
  --out ${CMAKE_CURRENT_SOURCE_DIR}/
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
message(NOTICE "-- Command classes generation completed")

### Create command class common library object

### THESE generated_files are legacy ones, will be removed later!!
### For now these are threated as common files, but they are not
file(GLOB generated_files src/generated/*.cpp)
file(GLOB_RECURSE implementation_files src/impl/**/*.cpp)

file(GLOB impl_headers src/impl/*)
file(GLOB private_impl_headers src/impl/*/private)

### Add component libraries

add_library(
  command_class_common STATIC
  ${generated_files}
  ${implementation_files}
  # Base class
  src/zwave_command_class_utils.cpp
  src/zwave_command_class_mqtt_utils.cpp
  src/zwave_command_class_base.cpp
)

target_link_libraries(
  command_class_common
  PUBLIC
    zpc_mqtt
    zpc_attribute_store
    zpc_attribute_resolver
    zwave_security_validation
    zwave_command_class_manager
    utils
    zwave_definitions 
    fmt::fmt
    mqtt_api
    component_connector
    granted_keys_resolver
    node_info_resolver
    device_interviewer
)

target_include_directories(
  command_class_common
  PUBLIC
    include 
    ${impl_headers}
  PRIVATE 
    src/base 
    src/generated
    ${private_impl_headers}
)

### Add generated sub directories
file(GLOB SUBDIRECTORIES LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/*")
set(SUBDIR_NAMES "")
foreach(SUBDIR ${SUBDIRECTORIES})
    if(IS_DIRECTORY ${SUBDIR})
        # Get just the directory name without the full path
        get_filename_component(DIR_NAME ${SUBDIR} NAME)
        if (DIR_NAME STREQUAL "test" OR DIR_NAME STREQUAL "src" OR DIR_NAME STREQUAL "include" OR DIR_NAME STREQUAL "data_model" OR DIR_NAME STREQUAL "assets" OR DIR_NAME STREQUAL "doc")
          continue()
        endif()

        if (NOT EXISTS "${SUBDIR}/CMakeLists.txt")
          continue()
        endif()

        if (NOT DIR_NAME STREQUAL "generated")
          list(APPEND SUBDIR_NAMES ${DIR_NAME})
        endif()

        add_subdirectory(${SUBDIR})
        math(EXPR COMPONENT_ATTRIBUTE_BASE "${COMPONENT_ATTRIBUTE_BASE} + ${COMPONENT_ATTRIBUTE_BASE_STEP}")
    endif()
endforeach()

set(INTERFACE_NAMES "")
foreach(SUBDIR ${SUBDIR_NAMES})
    list(APPEND INTERFACE_NAMES ${SUBDIR}_interface)
endforeach()

target_link_libraries(
  command_class_common
  PUBLIC
    ${INTERFACE_NAMES}
)


### Merge everything at the top level
add_library(
  command_classes
  # Entry point
  src/zwave_command_class_handlers_fixt.cpp
)

target_include_directories(command_classes
  PUBLIC
    include
    ${impl_headers}
  PRIVATE
    src/base 
    src/generated
    ${private_impl_headers}
)

target_link_libraries(command_classes
  PUBLIC
    command_class_common_generated
    command_class_common
  PRIVATE
    ${SUBDIR_NAMES}
    zwave_command_class_manager)

# Avoid "warning: relocation against" errors
target_compile_options(command_classes PRIVATE "-fPIC")
