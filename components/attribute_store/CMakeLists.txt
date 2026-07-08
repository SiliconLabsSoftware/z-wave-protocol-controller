# ZPC Attribute store Library (base functionality)
add_library(
  zpc_attribute_store_core OBJECT
  src/attribute_store.cpp
  src/attribute_store_callbacks.cpp
  src/attribute_store_helper.c
  src/attribute_store_node.cpp
  src/attribute_store_type_registration.cpp
  src/attribute_store_configuration.cpp
  src/attribute_store_validation.cpp
  src/attribute_store_process.cpp)

target_include_directories(
  zpc_attribute_store_core
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  PRIVATE src)

target_link_libraries(
  zpc_attribute_store_core PUBLIC datastore log config
                             timer threading)

# ZPC attribute store Library (Z-Wave specific extensions)
add_library(
  zpc_attribute_store
  src/zpc_attribute_store.c
  src/zpc_attribute_store_network_helper.c
  src/zpc_attribute_store_register_default_attribute_type_data.cpp
  src/zpc_attribute_store_type_registration.cpp
  src/zwave_association_toolbox.cpp
  src/zwave_utils.c
  src/zwave_frame_parser.cpp
  src/zwave_frame_generator.cpp
  src/zwave_frame_generator_base.cpp
  src/zwave_frame_generator_standalone.cpp
  )

target_include_directories(
  zpc_attribute_store
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/command_class_types>
  PRIVATE src)

target_link_libraries(
  zpc_attribute_store
  PUBLIC zpc_attribute_store_core
         utils zwave_controller network_monitor)

install(TARGETS zpc_attribute_store LIBRARY DESTINATION lib)

