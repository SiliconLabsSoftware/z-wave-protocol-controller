add_library(
  command_class_firmware_update_md STATIC
  src/command_class_firmware_update_md.cpp
  src/command_class_firmware_update_md_attribute_store.cpp
  src/command_class_firmware_update_md_mqtt.cpp
  generated/src/command_class_firmware_update_md_core.cpp
)

target_include_directories(command_class_firmware_update_md 
 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

# Link with required dependencies
target_link_libraries(
  command_class_firmware_update_md
  PUBLIC
    command_class_common
    command_class_firmware_update_md_interface
  PRIVATE
    zwave_tx
    zwave_tx_scheme_selector
    crc16_ccitt
    zpc_attribute_store
)

add_subdirectory(interface)