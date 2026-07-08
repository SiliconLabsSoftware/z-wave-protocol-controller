# Find nlohmann_json for MQTT API JSON parsing
find_path(nlohmann_json_include nlohmann/json.hpp REQUIRED)

add_library(
  ota_update_manager STATIC
  src/update_manager.cpp
  src/ota_mqtt_api.cpp
  src/ota_state_machine.cpp
  src/ota_image_store.cpp
  src/steps/ota_step_transfer_done.cpp
  src/steps/ota_step_failed.cpp
  src/steps/ota_step_start_upload.cpp
  src/steps/ota_step_upload_prepare.cpp
  src/steps/ota_step_publish_transfer_progress.cpp
  src/steps/ota_step_record_pending_transfer_abort.cpp
  src/steps/ota_step_process_device_transfer_outcome.cpp
  src/steps/ota_step_deliver_requested_firmware_chunks.cpp
  src/steps/ota_step_waiting_for_activation.cpp
  src/steps/ota_step_activating.cpp
  src/steps/ota_step_waiting_for_reconnect.cpp
  src/steps/ota_step_trigger_interview.cpp
)

target_include_directories(ota_update_manager
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc/steps>
  PRIVATE ${nlohmann_json_include}
)

target_link_libraries(ota_update_manager
  PUBLIC
    state_machine
    component_connector
    zpc_attribute_store
    mqtt_api
    zwave_definitions
    command_class_firmware_update_md_interface
    fmt::fmt
  PRIVATE
    log
    config
    crc16_ccitt
    zpc_attribute_resolver_core
    zwave_tx
    zwave_tx_scheme_selector
)
