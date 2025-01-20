/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#include "zwave_command_class_agi.h"
#include "zwave_command_classes_utils.h"
#include "zwave_command_class_association_helper.h"

// Generic includes
#include <assert.h>
#include <vector>
#include <set>

// Interface includes
#include "zwave_command_class_indices.h"
#include "attribute_store_defined_attribute_types.h"
#include "zpc_attribute_store.h"

// ZPC components includes
#include "zwave_command_handler.h"
#include "zwave_controller_utils.h"  // for EXTENDED_COMMAND_CLASS_IDENTIFIER_START
#include "zpc_attribute_store_network_helper.h"
#include "zwave_utils.h"

// Unify component includes
#include "attribute_store.h"
#include "attribute_store_helper.h"
#include "attribute_resolver.h"
#include "sl_log.h"

// Note, Multi Channel Association shares attributes with Association
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_ASSOCIATION_##type
constexpr char LOG_TAG[] = "command_class_agi";

///////////////////////////////////////////////////////////////////////////////
// Private variables
///////////////////////////////////////////////////////////////////////////////
namespace
{
// List of Command Class/Command pairs that, if sent by association groups,
// we want to make sure to try to associate ourselves to.
std::set<std::pair<zwave_command_class_t, zwave_command_t>> agi_listeners;
}  // namespace

// Function prototypes
static void zwave_command_class_agi_zpc_group_init();
static void zwave_command_class_agi_set_zpc_group_name(
  association_group_id_t group_id, const char *name, uint8_t name_size);
static void
  zwave_command_class_agi_set_zpc_group_profile(association_group_id_t group_id,
                                                agi_profile_t profile);

///////////////////////////////////////////////////////////////////////////////
// Private functions
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initializes and configures the AGI for the ZPC.
 *
 */
static void zwave_command_class_agi_zpc_configuration_init()
{
  zwave_command_class_agi_zpc_group_init();
  // Configure here the AGI of all groups supported by the ZPC.
  zwave_command_class_agi_set_zpc_group_name(LIFELINE_GROUP_ID,
                                             LIFELINE_NAME,
                                             sizeof(LIFELINE_NAME));
  zwave_command_class_agi_set_zpc_group_profile(LIFELINE_GROUP_ID,
                                                AGI_LIFELINE_PROFILE);
}

/**
 * @brief Initializes the ZPC group data and writes it down in the attribute store.
 *
 */
static void zwave_command_class_agi_zpc_group_init()
{
  attribute_store_node_t endpoint_node = get_zpc_endpoint_id_node(0);

  // Set our number of groups in the attribute store
  attribute_store_node_t supported_groupings_node
    = attribute_store_get_first_child_by_type(endpoint_node,
                                              ATTRIBUTE(SUPPORTED_GROUPINGS));

  if (supported_groupings_node == ATTRIBUTE_STORE_INVALID_NODE) {
    sl_log_debug(LOG_TAG,
                 "Creating Association Supported Groupings for the ZPC");
    supported_groupings_node
      = attribute_store_add_node(ATTRIBUTE(SUPPORTED_GROUPINGS), endpoint_node);
  }
  association_group_count_t supported_groupings = NUMBER_OF_SUPPORTED_GROUPS;
  attribute_store_set_reported(supported_groupings_node,
                               &supported_groupings,
                               sizeof(supported_groupings));

  // Ensure that all groups are created.
  for (association_group_id_t current_group = 1;
       current_group <= NUMBER_OF_SUPPORTED_GROUPS;
       current_group++) {
    attribute_store_node_t group_node
      = attribute_store_emplace(endpoint_node,
                                ATTRIBUTE(GROUP_ID),
                                &current_group,
                                sizeof(current_group));

    attribute_store_type_t attribute_list[] = {ATTRIBUTE(GROUP_NAME),
                                               ATTRIBUTE(GROUP_PROFILE),
                                               ATTRIBUTE(GROUP_COMMAND_LIST),
                                               ATTRIBUTE(GROUP_CONTENT),
                                               ATTRIBUTE(MAX_NODES_SUPPORTED)};
    attribute_store_add_if_missing(group_node,
                                   attribute_list,
                                   COUNT_OF(attribute_list));
    // Ensure that we have no empty group content
    attribute_store_node_t group_content_node
      = attribute_store_get_first_child_by_type(group_node,
                                                ATTRIBUTE(GROUP_CONTENT));
    if (attribute_store_is_value_defined(group_content_node, REPORTED_ATTRIBUTE)
        == false) {
      uint8_t marker = ASSOCIATION_MARKER;
      attribute_store_set_reported(group_content_node, &marker, sizeof(marker));
    }
    // Ensure that the Group Name / Profile / Command list are empty at init
    attribute_store_node_t group_name_node
      = attribute_store_get_first_child_by_type(group_node,
                                                ATTRIBUTE(GROUP_NAME));
    attribute_store_undefine_reported(group_name_node);
    attribute_store_node_t group_profile_node
      = attribute_store_get_first_child_by_type(group_node,
                                                ATTRIBUTE(GROUP_PROFILE));
    attribute_store_undefine_reported(group_profile_node);
    attribute_store_node_t group_command_list_node
      = attribute_store_get_first_child_by_type(group_node,
                                                ATTRIBUTE(GROUP_COMMAND_LIST));
    attribute_store_undefine_reported(group_command_list_node);
    attribute_store_node_t group_capacity_node
      = attribute_store_get_first_child_by_type(group_node,
                                                ATTRIBUTE(MAX_NODES_SUPPORTED));
    association_group_capacity_t group_capacity = MAX_SUPPORTED_NODES_PER_GROUP;
    attribute_store_set_reported(group_capacity_node,
                                 &group_capacity,
                                 sizeof(group_capacity));
  }
}

/**
 * @brief Sets the Association Group profile for the ZPC.
 *
 * @param group_id  Group ID to set the association group profile
 * @param profile   Group Profile to assign to the association group
 */
static void
  zwave_command_class_agi_set_zpc_group_profile(association_group_id_t group_id,
                                                agi_profile_t profile)
{
  attribute_store_node_t endpoint_node = get_zpc_endpoint_id_node(0);

  attribute_store_node_t group_id_node
    = attribute_store_get_node_child_by_value(endpoint_node,
                                              ATTRIBUTE(GROUP_ID),
                                              REPORTED_ATTRIBUTE,
                                              (uint8_t *)&group_id,
                                              sizeof(group_id),
                                              0);
  attribute_store_node_t group_profile_node
    = attribute_store_get_first_child_by_type(group_id_node,
                                              ATTRIBUTE(GROUP_PROFILE));

  attribute_store_set_reported(group_profile_node, &profile, sizeof(profile));
}

/**
 * @brief Sets the Association Group name for the ZPC.
 *
 * @param group_id        Group ID to set the association group name
 * @param name            The name to assign to the association group
 * @param name_size       The Length of the name string.
 */
static void zwave_command_class_agi_set_zpc_group_name(
  association_group_id_t group_id, const char *name, uint8_t name_size)
{
  attribute_store_node_t endpoint_node = get_zpc_endpoint_id_node(0);

  attribute_store_node_t group_id_node
    = attribute_store_get_node_child_by_value(endpoint_node,
                                              ATTRIBUTE(GROUP_ID),
                                              REPORTED_ATTRIBUTE,
                                              (uint8_t *)&group_id,
                                              sizeof(group_id),
                                              0);
  attribute_store_node_t group_name_node
    = attribute_store_get_first_child_by_type(group_id_node,
                                              ATTRIBUTE(GROUP_NAME));

  attribute_store_set_reported(group_name_node, name, name_size);
}

/**
 * @brief Find under which Association Group ID is an attribute store Node located.
 *
 * @param node    The node for which the Association Group ID is to be found
 * @returns 0 the node is not located under a valid Group ID. Else the value of
 *          the association group id.
 */
static association_group_id_t get_group_id(const attribute_store_node_t node)
{
  attribute_store_node_t group_id_node
    = attribute_store_get_first_parent_with_type(node, ATTRIBUTE(GROUP_ID));
  association_group_id_t group_id = 0;
  attribute_store_get_reported(group_id_node, &group_id, sizeof(group_id));
  return group_id;
}

/**
 * @brief Verifies if the an Attribute is located under a NodeID/EP that
 * supports the AGI Command CLass
 *
 * @param node      Attribute store node to verify if the NodeID/EP
 *                  parent supports AGI
 * @return true if AGI is supported, false otherwise (or unknown)
 */
static bool is_agi_supported_for_node(const attribute_store_node_t node)
{
  // Now, does the node support AGI ?
  zwave_node_id_t node_id         = 0;
  zwave_endpoint_id_t endpoint_id = 0;
  attribute_store_network_helper_get_zwave_ids_from_node(node,
                                                         &node_id,
                                                         &endpoint_id);
  return zwave_node_supports_command_class(COMMAND_CLASS_ASSOCIATION_GRP_INFO,
                                           node_id,
                                           endpoint_id);
}

///////////////////////////////////////////////////////////////////////////////
// Shared functions
///////////////////////////////////////////////////////////////////////////////
agi_profile_t
  zwave_command_class_agi_get_group_profile(zwave_node_id_t node_id,
                                            zwave_endpoint_id_t endpoint_id,
                                            association_group_id_t group_id)
{
  attribute_store_node_t endpoint_node
    = zwave_command_class_get_endpoint_id_node(node_id, endpoint_id);
  attribute_store_node_t group_node
    = attribute_store_get_node_child_by_value(endpoint_node,
                                              ATTRIBUTE(GROUP_ID),
                                              REPORTED_ATTRIBUTE,
                                              (uint8_t *)&group_id,
                                              sizeof(group_id),
                                              0);

  attribute_store_node_t profile_node
    = attribute_store_get_first_child_by_type(group_node,
                                              ATTRIBUTE(GROUP_PROFILE));

  agi_profile_t profile = AGI_RESERVED_PROFILE;
  attribute_store_get_reported(profile_node, &profile, sizeof(profile));
  return profile;
}

sl_status_t zwave_command_class_agi_add_group_commands(
  association_group_id_t group_id,
  zwave_command_class_t command_class,
  zwave_command_t command)
{
  attribute_store_node_t endpoint_node = get_zpc_endpoint_id_node(0);

  attribute_store_node_t group_id_node
    = attribute_store_get_node_child_by_value(endpoint_node,
                                              ATTRIBUTE(GROUP_ID),
                                              REPORTED_ATTRIBUTE,
                                              (uint8_t *)&group_id,
                                              sizeof(group_id),
                                              0);
  attribute_store_node_t group_command_list_node
    = attribute_store_get_first_child_by_type(group_id_node,
                                              ATTRIBUTE(GROUP_COMMAND_LIST));
  if (group_command_list_node == ATTRIBUTE_STORE_INVALID_NODE) {
    return SL_STATUS_NOT_FOUND;
  }

  uint8_t command_list[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH] = {};
  uint8_t command_list_length                                = 0;
  attribute_store_get_node_attribute_value(group_command_list_node,
                                           REPORTED_ATTRIBUTE,
                                           command_list,
                                           &command_list_length);

  if (true
      == is_command_in_array(command_class,
                             command,
                             command_list,
                             command_list_length)) {
    // Already registered, no need to do anything here
    return SL_STATUS_OK;
  }

  // Else we have to add it at the end.
  if (command_class >= EXTENDED_COMMAND_CLASS_IDENTIFIER_START) {
    if (command_list_length > (sizeof(command_list) - 3)) {
      return SL_STATUS_FAIL;
    }
    command_list[command_list_length]     = (uint8_t)(command_class >> 8);
    command_list[command_list_length + 1] = (uint8_t)(command_class & 0xFF);
    command_list[command_list_length + 2] = command;
    command_list_length += 3;
  } else {
    if (command_list_length > (sizeof(command_list) - 2)) {
      return SL_STATUS_FAIL;
    }
    command_list[command_list_length]     = (uint8_t)command_class;
    command_list[command_list_length + 1] = command;
    command_list_length += 2;
  }

  return attribute_store_set_reported(group_command_list_node,
                                      command_list,
                                      command_list_length);
}

bool zwave_command_class_agi_is_command_allowed_for_group(
  association_group_id_t group_id,
  zwave_command_class_t command_class,
  zwave_command_t command)
{
  attribute_store_node_t endpoint_node = get_zpc_endpoint_id_node(0);

  attribute_store_node_t group_id_node
    = attribute_store_get_node_child_by_value(endpoint_node,
                                              ATTRIBUTE(GROUP_ID),
                                              REPORTED_ATTRIBUTE,
                                              (uint8_t *)&group_id,
                                              sizeof(group_id),
                                              0);
  attribute_store_node_t group_command_list_node
    = attribute_store_get_first_child_by_type(group_id_node,
                                              ATTRIBUTE(GROUP_COMMAND_LIST));

  uint8_t command_list[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH] = {};
  uint8_t command_list_length                                = 0;
  attribute_store_get_node_attribute_value(group_command_list_node,
                                           REPORTED_ATTRIBUTE,
                                           command_list,
                                           &command_list_length);
  return is_command_in_array(command_class,
                             command,
                             command_list,
                             command_list_length);
}

void zwave_command_class_agi_request_to_establish_association(
  zwave_command_class_t command_class, zwave_command_t command)
{
  // The list is only additive, we never clear it.
  agi_listeners.insert(std::make_pair(command_class, command));
}

bool zwave_command_class_agi_group_contains_listeners(
  zwave_node_id_t node_id,
  zwave_endpoint_id_t endpoint_id,
  association_group_id_t group_id)
{
  attribute_store_node_t command_list_node
    = get_group_child_node(node_id,
                           endpoint_id,
                           group_id,
                           ATTRIBUTE(GROUP_COMMAND_LIST));
  uint8_t group_command_list[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH] = {};
  uint8_t group_command_list_length                                = 0;
  attribute_store_get_node_attribute_value(command_list_node,
                                           REPORTED_ATTRIBUTE,
                                           group_command_list,
                                           &group_command_list_length);

  // Is there a Command Class / Command pair that we want to listen to ?
  for (const auto &[command_class, command]: agi_listeners) {
    if (true
        == is_command_in_array(command_class,
                               command,
                               group_command_list,
                               group_command_list_length)) {
      sl_log_debug(LOG_TAG,
                   "We have listeners for NodeID %d:%d - "
                   "Group ID %d. Recommending to establish association.",
                   node_id,
                   endpoint_id,
                   group_id);
      return true;
    }
  }

  sl_log_debug(LOG_TAG,
               "We are not interested in Association Group ID %d "
               "for NodeID %d:%d.",
               group_id,
               node_id,
               endpoint_id);
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// Attribute resolution functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_agi_info_get(attribute_store_node_t node,
                                                    uint8_t *frame,
                                                    uint16_t *frame_length)
{
  // Find the group ID
  association_group_id_t group_id = get_group_id(node);
  if (group_id == 0) {
    *frame_length = 0;
    return SL_STATUS_FAIL;
  }
  if (!is_agi_supported_for_node(node)) {
    // No AGI data is available: We assume everything is a lifeline
    const agi_profile_t profile = AGI_LIFELINE_PROFILE;
    attribute_store_set_reported(node, &profile, sizeof(profile));
    *frame_length = 0;
    return SL_STATUS_ALREADY_EXISTS;
  }

  auto *get_frame
    = reinterpret_cast<ZW_ASSOCIATION_GROUP_INFO_GET_FRAME *>(frame);
  get_frame->cmdClass = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
  get_frame->cmd      = ASSOCIATION_GROUP_INFO_GET;
  // Do not use List mode, it does not play well with S2.
  get_frame->properties1        = RESERVED_BYTE;
  get_frame->groupingIdentifier = static_cast<uint8_t>(group_id);
  *frame_length                 = sizeof(ZW_ASSOCIATION_GROUP_INFO_GET_FRAME);
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_agi_name_get(attribute_store_node_t node,
                                                    uint8_t *frame,
                                                    uint16_t *frame_length)
{
  // Find the group ID
  association_group_id_t group_id = get_group_id(node);
  if (group_id == 0) {
    *frame_length = 0;
    return SL_STATUS_FAIL;
  }
  if (is_agi_supported_for_node(node)) {
    auto *get_frame
      = reinterpret_cast<ZW_ASSOCIATION_GROUP_NAME_GET_FRAME *>(frame);
    get_frame->cmdClass           = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
    get_frame->cmd                = ASSOCIATION_GROUP_NAME_GET;
    get_frame->groupingIdentifier = static_cast<uint8_t>(group_id);
    *frame_length                 = sizeof(ZW_ASSOCIATION_GROUP_NAME_GET_FRAME);
    return SL_STATUS_OK;
  }
  // if not supported, store an empty string
  const uint8_t value = '\0';
  attribute_store_set_reported(node, &value, sizeof(value));
  *frame_length = 0;
  return SL_STATUS_ALREADY_EXISTS;
}
static sl_status_t zwave_command_class_agi_command_list_get(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
{
  // Find the group ID
  association_group_id_t group_id = get_group_id(node);
  if (group_id == 0) {
    *frame_length = 0;
    return SL_STATUS_FAIL;
  }
  if (is_agi_supported_for_node(node)) {
    auto *get_frame
      = reinterpret_cast<ZW_ASSOCIATION_GROUP_COMMAND_LIST_GET_FRAME *>(frame);
    get_frame->cmdClass           = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
    get_frame->cmd                = ASSOCIATION_GROUP_COMMAND_LIST_GET;
    get_frame->properties1        = RESERVED_BYTE;
    get_frame->groupingIdentifier = static_cast<uint8_t>(group_id);
    *frame_length = sizeof(ZW_ASSOCIATION_GROUP_COMMAND_LIST_GET_FRAME);
    return SL_STATUS_OK;
  }
  // if not supported, store 0x00 here
  const uint8_t value = 0;
  attribute_store_set_reported(node, &value, sizeof(value));
  *frame_length = 0;
  return SL_STATUS_ALREADY_EXISTS;
}
///////////////////////////////////////////////////////////////////////////////
// Command Handler functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_agi_handle_group_info_report_command(
  const zwave_controller_connection_info_t *connection,
  const uint8_t *frame,
  uint16_t frame_length)
{
  if (frame_length < 3) {
    return SL_STATUS_OK;
  }

  const uint8_t start_index = 2;
  uint8_t group_count = frame[start_index] & AGI_INFO_REPORT_GROUP_COUNT_MASK;

  for (uint8_t i = 0; i < group_count; i++) {
    if (frame_length <= (start_index + (i * 7) + 4)) {
      sl_log_debug(LOG_TAG, "Frame too short, aborting the AGI frame parsing");
      return SL_STATUS_OK;
    }
    association_group_id_t group_id = frame[start_index + (i * 7) + 1];
    agi_profile_t profile           = frame[start_index + (i * 7) + 3] << 8;
    profile |= frame[start_index + (i * 7) + 4];
    attribute_store_node_t group_profile_node
      = get_group_child_node(connection->remote.node_id,
                             connection->remote.endpoint_id,
                             group_id,
                             ATTRIBUTE(GROUP_PROFILE));
    attribute_store_set_reported(group_profile_node, &profile, sizeof(profile));
  }
  return SL_STATUS_OK;
}
static sl_status_t
  zwave_command_class_agi_handle_group_command_list_report_command(
    const zwave_controller_connection_info_t *connection,
    const uint8_t *frame,
    uint16_t frame_length)
{
  if (frame_length
      < sizeof(ZW_ASSOCIATION_GROUP_COMMAND_LIST_REPORT_1BYTE_FRAME)) {
    return SL_STATUS_OK;
  }
  const auto *report_frame = reinterpret_cast<
    const ZW_ASSOCIATION_GROUP_COMMAND_LIST_REPORT_1BYTE_FRAME *>(frame);
  // Sanity check frame length
  if (report_frame->listLength
      > frame_length
          - offsetof(ZW_ASSOCIATION_GROUP_COMMAND_LIST_REPORT_1BYTE_FRAME,
                     command1)) {
    sl_log_warning(LOG_TAG,
                   "Invalid frame, listLength is %d, framelength is %d",
                   report_frame->listLength,
                   frame_length);
    return SL_STATUS_OK;
  }
  attribute_store_node_t group_cmd_list_node
    = get_group_child_node(connection->remote.node_id,
                           connection->remote.endpoint_id,
                           report_frame->groupingIdentifier,
                           ATTRIBUTE(GROUP_COMMAND_LIST));
  attribute_store_set_reported(group_cmd_list_node,
                               &report_frame->command1,
                               report_frame->listLength);
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_agi_handle_group_name_report(
  const zwave_controller_connection_info_t *connection,
  const uint8_t *frame,
  uint16_t frame_length)
{
  if (frame_length < sizeof(ZW_ASSOCIATION_GROUP_NAME_REPORT_1BYTE_FRAME)) {
    return SL_STATUS_OK;
  }
  const auto *report_frame
    = reinterpret_cast<const ZW_ASSOCIATION_GROUP_NAME_REPORT_1BYTE_FRAME *>(
      frame);
  // Sanity check frame length
  if (report_frame->lengthOfName
      > frame_length
          - offsetof(ZW_ASSOCIATION_GROUP_NAME_REPORT_1BYTE_FRAME, name1)) {
    sl_log_warning(LOG_TAG,
                   "Invalid frame, lengthOfName is %d, framelength is %d",
                   report_frame->lengthOfName,
                   frame_length);
    return SL_STATUS_OK;
  }
  attribute_store_node_t group_name_node
    = get_group_child_node(connection->remote.node_id,
                           connection->remote.endpoint_id,
                           report_frame->groupingIdentifier,
                           ATTRIBUTE(GROUP_NAME));
  // make '\0' termination of name
  std::vector<char> name;
  std::copy(&report_frame->name1,
            &report_frame->name1 + report_frame->lengthOfName,
            std::back_inserter(name));
  name.push_back('\0');
  attribute_store_set_reported(group_name_node, name.data(), name.size());
  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Supported Command Handler functions
///////////////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_agi_handle_group_name_get(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame)
{
  if (connection_info->local.is_multicast) {
    return SL_STATUS_OK;
  }

  attribute_store_node_t zpc_ep0_node
    = get_zpc_endpoint_id_node(connection_info->local.endpoint_id);
  const auto *get_frame
    = reinterpret_cast<const ZW_ASSOCIATION_GROUP_NAME_GET_FRAME *>(frame);
  association_group_id_t group_identifier = get_frame->groupingIdentifier;
  attribute_store_node_t group_identifier_node
    = attribute_store_get_node_child_by_value(zpc_ep0_node,
                                              ATTRIBUTE(GROUP_ID),
                                              REPORTED_ATTRIBUTE,
                                              &group_identifier,
                                              sizeof(group_identifier),
                                              0);
  if (group_identifier_node != ATTRIBUTE_STORE_INVALID_NODE) {
    attribute_store_node_t group_name_node
      = attribute_store_get_first_child_by_type(group_identifier_node,
                                                ATTRIBUTE(GROUP_NAME));
    association_group_name_report_frame_t report = {};
    report.command_class       = COMMAND_CLASS_ASSOCIATION_GRP_INFO_V3;
    report.command             = ASSOCIATION_GROUP_NAME_REPORT_V3;
    report.grouping_identifier = group_identifier;
    uint8_t name_size          = 0;
    attribute_store_get_node_attribute_value(group_name_node,
                                             REPORTED_ATTRIBUTE,
                                             report.name,
                                             &name_size);
    report.name_length = name_size - 1;
    uint8_t report_length
      = offsetof(association_group_name_report_frame_t, name) + name_size - 1;
    return zwave_command_class_send_report(connection_info,
                                           report_length,
                                           (uint8_t *)&report);
  }
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_agi_handle_group_info_get(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame)
{
  if (connection_info->local.is_multicast) {
    return SL_STATUS_OK;
  }
  attribute_store_node_t zpc_ep0_node
    = get_zpc_endpoint_id_node(connection_info->local.endpoint_id);
  const auto *get_frame
    = reinterpret_cast<const ZW_ASSOCIATION_GROUP_INFO_GET_FRAME *>(frame);
  uint8_t list_mode
    = get_frame->properties1
      & ASSOCIATION_GROUP_INFO_GET_PROPERTIES1_LIST_MODE_BIT_MASK;
  // Note that the ZPC Association Group Information does not change on the fly.
  // Due to that we set the Dynamic Info field to zero.
  uint8_t dynamic_info = 0x00;
  uint8_t group_count  = 0x00;

  association_group_info_report_frame_t report_frame = {};
  report_frame.command_class = COMMAND_CLASS_ASSOCIATION_GRP_INFO_V3;
  report_frame.command       = ASSOCIATION_GROUP_INFO_REPORT;
  uint8_t report_frame_size;

  // If List Mode is set to 0, a receiving node MUST advertise the properties
  // of the association group identified by the Grouping Identifier.
  if (list_mode == 0x00) {
    association_group_id_t group_identifier = get_frame->groupingIdentifier;
    agi_profile_t profile = zwave_command_class_agi_get_group_profile(
      connection_info->local.node_id,
      connection_info->local.endpoint_id,
      group_identifier);

    // If List Mode is set to 0, the Group Count field MUST be set to 1.
    group_count             = 0x01;
    report_frame.properties = list_mode | dynamic_info | group_count;
    report_frame.vg_ass_group_info[0].groupingIdentifier
      = static_cast<uint8_t>(group_identifier);
    report_frame.vg_ass_group_info[0].mode       = 0x00;
    report_frame.vg_ass_group_info[0].reserved   = RESERVED_BYTE;
    report_frame.vg_ass_group_info[0].eventCode1 = 0x00;
    report_frame.vg_ass_group_info[0].eventCode2 = 0x00;
    report_frame.vg_ass_group_info[0].profile1
      = static_cast<uint8_t>(profile >> 8);
    report_frame.vg_ass_group_info[0].profile2
      = static_cast<uint8_t>(profile & 0xFF);
    report_frame_size
      = offsetof(association_group_info_report_frame_t, vg_ass_group_info)
        + sizeof(VG_ASSOCIATION_GROUP_INFO_REPORT_VG);
  } else {
    // First get the number of child per ZPC endpoints
    size_t nm_childs = attribute_store_get_node_child_count(zpc_ep0_node);
    attribute_store_node_t group_id_node = ATTRIBUTE_STORE_INVALID_NODE;
    association_group_id_t group_id;
    agi_profile_t group_profile;
    for (size_t i = 0; i < nm_childs; i++) {
      group_id_node
        = attribute_store_get_node_child_by_type(zpc_ep0_node,
                                                 ATTRIBUTE(GROUP_ID),
                                                 i);
      if (group_id_node != ATTRIBUTE_STORE_INVALID_NODE) {
        // Read Group ID and Group profile of each Group ID contents
        attribute_store_get_reported(group_id_node,
                                     &group_id,
                                     sizeof(group_id));

        group_profile = zwave_command_class_agi_get_group_profile(
          connection_info->local.node_id,
          connection_info->local.endpoint_id,
          group_id);
        report_frame.vg_ass_group_info[group_count].groupingIdentifier
          = static_cast<uint8_t>(group_id);
        report_frame.vg_ass_group_info[group_count].mode       = 0x00;
        report_frame.vg_ass_group_info[group_count].reserved   = RESERVED_BYTE;
        report_frame.vg_ass_group_info[group_count].eventCode1 = 0x00;
        report_frame.vg_ass_group_info[group_count].eventCode2 = 0x00;
        report_frame.vg_ass_group_info[group_count].profile1
          = static_cast<uint8_t>(group_profile >> 8);
        report_frame.vg_ass_group_info[group_count].profile2
          = static_cast<uint8_t>(group_profile & 0xFF);
        group_count++;
      }
    }
    report_frame.properties
      = ASSOCIATION_GROUP_INFO_REPORT_PROPERTIES1_LIST_MODE_BIT_MASK_V2
        | dynamic_info | group_count;
    report_frame_size
      = offsetof(association_group_info_report_frame_t, vg_ass_group_info)
        + (sizeof(VG_ASSOCIATION_GROUP_INFO_REPORT_VG) * group_count);
  }

  return zwave_command_class_send_report(connection_info,
                                         report_frame_size,
                                         (uint8_t *)&report_frame);
}

static sl_status_t zwave_command_class_agi_handle_group_command_list_get(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame)
{
  if (connection_info->local.is_multicast) {
    return SL_STATUS_OK;
  };
  attribute_store_node_t zpc_ep0_node
    = get_zpc_endpoint_id_node(connection_info->local.endpoint_id);
  const auto *get_frame
    = reinterpret_cast<const ZW_ASSOCIATION_GROUP_COMMAND_LIST_GET_FRAME *>(
      frame);
  association_group_id_t group_id = get_frame->groupingIdentifier;
  attribute_store_node_t group_id_node
    = attribute_store_get_node_child_by_value(zpc_ep0_node,
                                              ATTRIBUTE(GROUP_ID),
                                              REPORTED_ATTRIBUTE,
                                              &group_id,
                                              sizeof(group_id),
                                              0);
  if (group_id_node != ATTRIBUTE_STORE_INVALID_NODE) {
    attribute_store_node_t group_command_list_node
      = attribute_store_get_first_child_by_type(group_id_node,
                                                ATTRIBUTE(GROUP_COMMAND_LIST));
    association_group_command_list_report_frame_t report_frame = {};
    report_frame.command_class       = COMMAND_CLASS_ASSOCIATION_GRP_INFO_V3;
    report_frame.command             = ASSOCIATION_GROUP_COMMAND_LIST_REPORT_V3;
    report_frame.grouping_identifier = group_id;
    uint8_t command_list_size        = 0;
    attribute_store_get_node_attribute_value(group_command_list_node,
                                             REPORTED_ATTRIBUTE,
                                             report_frame.command_list,
                                             &command_list_size);
    report_frame.list_length = command_list_size;

    uint8_t report_length
      = offsetof(association_group_command_list_report_frame_t, command_list)
        + command_list_size;
    return zwave_command_class_send_report(connection_info,
                                           report_length,
                                           (uint8_t *)&report_frame);
  }

  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_agi_support_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  if (frame_data[COMMAND_CLASS_INDEX]
      != COMMAND_CLASS_ASSOCIATION_GRP_INFO_V3) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  switch (frame_data[COMMAND_INDEX]) {
    case ASSOCIATION_GROUP_NAME_GET_V3:
      return zwave_command_class_agi_handle_group_name_get(connection_info,
                                                           frame_data);

    case ASSOCIATION_GROUP_INFO_GET_V3:
      return zwave_command_class_agi_handle_group_info_get(connection_info,
                                                           frame_data);

    case ASSOCIATION_GROUP_COMMAND_LIST_GET_V3:
      return zwave_command_class_agi_handle_group_command_list_get(
        connection_info,
        frame_data);

    default:
      return SL_STATUS_NOT_SUPPORTED;
  }
}

static sl_status_t zwave_command_class_agi_control_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  if (frame_data[COMMAND_CLASS_INDEX] != COMMAND_CLASS_ASSOCIATION_GRP_INFO) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  switch (frame_data[COMMAND_INDEX]) {
    case ASSOCIATION_GROUP_NAME_REPORT_V3:
      return zwave_command_class_agi_handle_group_name_report(connection_info,
                                                              frame_data,
                                                              frame_length);

    case ASSOCIATION_GROUP_INFO_REPORT:
      return zwave_command_class_agi_handle_group_info_report_command(
        connection_info,
        frame_data,
        frame_length);

    case ASSOCIATION_GROUP_COMMAND_LIST_REPORT:
      return zwave_command_class_agi_handle_group_command_list_report_command(
        connection_info,
        frame_data,
        frame_length);
      return SL_STATUS_NOT_SUPPORTED;

    default:
      return SL_STATUS_NOT_SUPPORTED;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Attribute update callbacks
///////////////////////////////////////////////////////////////////////////////
/**
 * @brief Reacts to updates Assocation Group ID updates and creates AGI data
 *
 * @param updated_node    Association Group ID node that was updated.
 * @param change          Attribute Store changed peformed on the node.
 */
static void
  zwave_command_class_association_group_information_on_group_id_attribute_update(
    attribute_store_node_t updated_node, attribute_store_change_t change)
{
  if (change == ATTRIBUTE_DELETED) {
    return;
  }

  // Check that we have the right type of attribute.
  assert(ATTRIBUTE(GROUP_ID) == attribute_store_get_node_type(updated_node));

  // Check if it is our own (get the parent NodeID and verify if it is get_zpc_node_id_node)
  // If yes, we do not create or resolve AGI information for ourselves.
  if (get_zpc_node_id_node()
      == attribute_store_get_first_parent_with_type(updated_node,
                                                    ATTRIBUTE_NODE_ID)) {
    return;
  }

  association_group_id_t group_id = 0;
  attribute_store_get_reported(updated_node, &group_id, sizeof(group_id));

  if (group_id == 0) {
    // Wait for the Group Identifier to be written down.
    return;
  }

  // Let the rest of the command class perform the job.
  attribute_store_type_t attribute_list[] = {ATTRIBUTE(GROUP_NAME),
                                             ATTRIBUTE(GROUP_PROFILE),
                                             ATTRIBUTE(GROUP_COMMAND_LIST)};
  attribute_store_add_if_missing(updated_node,
                                 attribute_list,
                                 COUNT_OF(attribute_list));
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_command_class_agi_init()
{
  // Initializes ZPC's Association Groups
  zwave_command_class_agi_zpc_configuration_init();

  // Register Attribute Store callbacks.
  attribute_store_register_callback_by_type(
    zwave_command_class_association_group_information_on_group_id_attribute_update,
    ATTRIBUTE_COMMAND_CLASS_ASSOCIATION_GROUP_ID);

  // Attribute resolver rules
  attribute_resolver_register_rule(ATTRIBUTE(GROUP_PROFILE),
                                   nullptr,
                                   &zwave_command_class_agi_info_get);
  attribute_resolver_register_rule(ATTRIBUTE(GROUP_NAME),
                                   nullptr,
                                   &zwave_command_class_agi_name_get);
  attribute_resolver_register_rule(ATTRIBUTE(GROUP_COMMAND_LIST),
                                   nullptr,
                                   &zwave_command_class_agi_command_list_get);

  // Register our handler to the Z-Wave CC framework:
  zwave_command_handler_t handler = {};
  handler.support_handler         = zwave_command_class_agi_support_handler;
  handler.control_handler         = zwave_command_class_agi_control_handler;
  handler.minimal_scheme     = ZWAVE_CONTROLLER_ENCAPSULATION_NETWORK_SCHEME;
  handler.command_class      = COMMAND_CLASS_ASSOCIATION_GRP_INFO_V3;
  handler.version            = ASSOCIATION_GRP_INFO_VERSION_V3;
  handler.command_class_name = "Association Group Info (AGI)";
  handler.manual_security_validation = false;

  zwave_command_handler_register_handler(handler);

  return SL_STATUS_OK;
}