# Find nlohmann_json for MQTT API (same pattern as discovery/network_manager)
find_path(nlohmann_json_include nlohmann/json.hpp REQUIRED)

add_library(
  device_interviewer STATIC
  src/device_interviewer.cpp
  src/device_interviewer_mqtt_api.cpp
  src/interview_state_machine.cpp
  src/steps/interview_step_node_information.cpp
  src/steps/interview_step_s0_commands_supported.cpp
  src/steps/interview_step_s2_commands_supported.cpp
  src/steps/interview_step_version_report.cpp
  src/steps/interview_step_version_capabilities.cpp
  src/steps/interview_step_version_zwave_software.cpp
  src/steps/interview_step_version_get.cpp
  src/steps/interview_step_prepare_version_cc_list.cpp
  src/steps/interview_step_version_cc_sequence.cpp
  src/steps/interview_step_zwaveplus_info.cpp
  src/steps/interview_step_wake_up.cpp
  src/steps/interview_step_check_multi_channel_support.cpp
  src/steps/interview_step_mc_endpoint_get.cpp
  src/steps/interview_step_get_number_of_endpoints.cpp
  src/steps/interview_step_get_endpoint_capabilities.cpp
  src/steps/interview_step_get_endpoint_s2_capabilities.cpp
  src/steps/interview_step_get_endpoint_s0_capabilities.cpp
  src/steps/interview_step_multi_channel_association_supported_groupings.cpp
  src/steps/interview_step_multi_channel_association_supported_groupings_count.cpp
  src/steps/interview_step_association_supported_groupings.cpp
  src/steps/interview_step_association_get.cpp
  src/steps/interview_step_lifeline_set.cpp
  src/steps/interview_step_lifeline_validate.cpp
  src/steps/interview_step_post_validate_lifeline.cpp
  src/steps/interview_step_get_agi_group_count.cpp
  src/steps/interview_step_agi_group_name_get.cpp
  src/steps/interview_step_agi_group_info_get.cpp
  src/steps/interview_step_agi_group_command_list_get.cpp
  src/steps/interview_step_prepare_endpoint_versions.cpp
  src/steps/interview_step_get_endpoint_zwaveplus_info.cpp
  src/steps/interview_step_endpoint_association_iterator.cpp
  src/steps/interview_step_completed.cpp
)

target_include_directories(device_interviewer
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc/steps>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc/generated>
  PRIVATE ${nlohmann_json_include}
)

target_link_libraries(device_interviewer
  PUBLIC
  state_machine
  zpc_attribute_store
  zpc_attribute_resolver
  zwave_definitions
  component_connector
  command_class_association_interface
  command_class_multi_channel_association_interface
  command_class_association_grp_info_interface
  command_class_s2
  command_class_s0
  command_class_multi_channel_interface
  command_class_zwaveplus_info_interface
  command_class_wake_up_interface
  fmt::fmt
  mqtt_api
  PRIVATE
  attribute_timeouts
  config)

configure_file(inc/device_interviewer_events.hpp.in 
  ${CMAKE_CURRENT_LIST_DIR}/inc/generated/device_interviewer_events.hpp)

configure_file(inc/device_interviewer_attribute_store.hpp.in 
  ${CMAKE_CURRENT_LIST_DIR}/inc/generated/device_interviewer_attribute_store.hpp)