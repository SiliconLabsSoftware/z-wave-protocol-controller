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

// Includes from this component
#include "zwave_command_class_notification.h"
#include "zwave_command_class_notification_types.h"
#include "notification_command_class_defined_notifications.h"
#include "zwave_command_class_indices.h"
#include "zwave_command_classes_utils.h"
#include "zwave_command_class_agi.h"

// Includes from other components
#include "sl_log.h"
#include "zwave_unid.h"
#include "zpc_attribute_store_network_helper.h"
#include "attribute_store_helper.h"
#include "attribute_store_defined_attribute_types.h"
#include "ZW_classcmd.h"
#include "zwave_command_handler.h"
#include "attribute_resolver.h"
#include "zwave_controller_utils.h"
#include "zwave_utils.h"
#include "zwave_frame_generator.hpp"
#include "zpc_attribute_store.h"
#include "process.h"
#include "etimer.h"

// Generic includes
#include <assert.h>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <attribute.hpp>

// Attribute macro, shortening those long defines for attribute types:
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_NOTIFICATION_##type

// Log tag
static constexpr char LOG_TAG[]          = "zwave_command_class_notification";
static constexpr char NOTIFICATION_TAG[] = "NOTIFICATION";
static constexpr unsigned int MAX_SUPPORTED_NOTIFICATION_TYPES  = 24;
static constexpr unsigned int MAX_SUPPORTED_NOTIFICATION_STATES = 256;
static constexpr unsigned int NOTIFICATION_REPORT_EVENT_STATE_PARAMETER_OFFSET
  = 9;
static constexpr uint8_t PUSH_MODE                    = 0x00;
static constexpr uint8_t PULL_MODE                    = 0x01;
static constexpr uint8_t PUSH_REPORT_STATUS_DISABLED  = 0x00;
static constexpr uint8_t PUSH_REPORT_STATUS_ENABLED   = 0xFF;
static constexpr uint8_t PULL_REPORT_STATUS_NOT_EMPTY = 0x00;
static constexpr uint8_t PULL_REPORT_STATUS_EMPTY     = 0xFE;

static constexpr uint32_t PULL_NODE_PROBE_INTERVAL
  = 10800000;  //in msec (3h * 60m * 60s * 1000ms)

enum class mode_discovery_state_t : uint8_t {
  PUSH_MODE_DETECTED        = 0x00,
  PULL_MODE_DETECTED        = 0x01,
  AGI_TEST_PENDING          = 0x02,
  NOTIFICATION_TEST_PENDING = 0x03,
  UNKNOWN                   = 0xFF
};

struct event_queue {
  uint8_t type;
  uint8_t event;
  uint8_t seq;
};

namespace
{
// Private variables
static mode_discovery_state_t
  discovery_state;  //NOSONAR : false positive since it is warped in a namespace
// probe timer, that will ensure that we probe all pull nodes
// at least this interval.
static struct etimer
  notification_probe_timer;  //NOSONAR : false positive since it is warped in a namespace
static std::map<attribute_store::attribute, std::vector<struct event_queue>>
  pull_mode_queue;
}  // namespace

// Cpp helpers
namespace
{
zwave_frame_generator frame_generator(
  COMMAND_CLASS_NOTIFICATION_V8);  //NOSONAR - false positive since it is warped in a namespace
}

PROCESS(zwave_command_class_notification_process,
        "zwave_command_class_notification_process");

using namespace attribute_store;

#include "zwave_command_class_notification_types.inc"

//////////////////////////////////////////////////////////////////////
// Discovery helpers
//////////////////////////////////////////////////////////////////////
void zwave_command_class_continue_mode_discovery(attribute_store_node_t node)
{
  // we don't need notification on further resolutions if any
  attribute_resolver_clear_resolution_listener(
    node,
    zwave_command_class_continue_mode_discovery);

  attribute_store::attribute supported_events_node(node);
  auto notification_type_node = supported_events_node.parent();
  notification_type_node.set_desired<uint8_t>(
    notification_type_node.reported<uint8_t>());
}
void zwave_command_class_agi_test_perform_discovery(attribute_store_node_t node)
{
  attribute_store::attribute resolved_node(node);
  auto ep_node = resolved_node.first_parent(ATTRIBUTE_ENDPOINT_ID);

  auto group_count
    = ep_node
        .child_by_type(ATTRIBUTE_COMMAND_CLASS_ASSOCIATION_SUPPORTED_GROUPINGS)
        .reported<uint32_t>();
  auto mode_node = ep_node.child_by_type(ATTRIBUTE(MODE));

  discovery_state = mode_discovery_state_t::AGI_TEST_PENDING;
  for (uint32_t i = 1; i <= group_count; i++) {
    auto group_node = ep_node.child_by_type_and_value<uint8_t>(
      ATTRIBUTE_COMMAND_CLASS_ASSOCIATION_GROUP_ID,
      i);
    if (!group_node.is_valid()) {
      sl_log_warning(LOG_TAG, "Group with ID %u missing in attribute store", i);
      return;
    }
    if (!group_node.reported_exists()) {
      attribute_resolver_clear_resolution_listener(
        node,
        zwave_command_class_agi_test_perform_discovery);
      attribute_resolver_set_resolution_listener(
        group_node,
        zwave_command_class_agi_test_perform_discovery);
    }
    auto group_cmdlist_node = group_node.child_by_type(
      ATTRIBUTE_COMMAND_CLASS_ASSOCIATION_GROUP_COMMAND_LIST);
    if (!group_cmdlist_node.is_valid()
        || !group_cmdlist_node.reported_exists()) {
      sl_log_warning(LOG_TAG, "Group Cmd List info missing for group %u", i);
      return;
    }
    auto command_list = group_cmdlist_node.reported<std::vector<uint8_t>>();

    if (is_command_in_array(COMMAND_CLASS_NOTIFICATION_V8,
                            NOTIFICATION_REPORT_V3,
                            command_list.data(),
                            command_list.size())) {
      sl_log_error(LOG_TAG,
                   "Found NOTIFICATION_REPORT_V3 report in group [%u] hence "
                   "detecting mode as PUSH",
                   i);
      discovery_state = mode_discovery_state_t::PUSH_MODE_DETECTED;
      mode_node.set_reported<uint8_t>(PUSH_MODE);
      attribute_resolver_clear_resolution_listener(
        node,
        zwave_command_class_agi_test_perform_discovery);
      return;
    }
  }
  sl_log_error(LOG_TAG,
               "NOTIFICATION_REPORT_V3 not found in any AGI groups, "
               "detecting mode as PULL");
  discovery_state = mode_discovery_state_t::PULL_MODE_DETECTED;
  mode_node.set_reported<uint8_t>(PULL_MODE);
  ep_node.emplace_node(ATTRIBUTE(PROBE_ACTIVE));
  attribute_resolver_clear_resolution_listener(
    node,
    zwave_command_class_agi_test_perform_discovery);
}

void zwave_command_class_notification_pull_push_discovery(
  zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id)
{
  unid_t node_unid;
  zwave_unid_from_node_id(node_id, node_unid);

  // Get the corresponding Attribute Store entry for this unid / endpoint:
  attribute_store::attribute ep_node
    = attribute_store_network_helper_get_endpoint_node(node_unid, endpoint_id);

  auto association_support
    = zwave_node_supports_command_class(COMMAND_CLASS_ASSOCIATION,
                                        node_id,
                                        endpoint_id);
  auto agi_support
    = zwave_node_supports_command_class(COMMAND_CLASS_ASSOCIATION_GRP_INFO_V3,
                                        node_id,
                                        endpoint_id);

  if (!association_support) {
    sl_log_debug(LOG_TAG,
                 "Association CC not supported, detecting mode as PULL");

    auto mode_node = ep_node.child_by_type(ATTRIBUTE(MODE));
    mode_node.set_reported<uint8_t>(PULL_MODE);
    ep_node.emplace_node(ATTRIBUTE(PROBE_ACTIVE));
    discovery_state = mode_discovery_state_t::PULL_MODE_DETECTED;
    return;
  }
  if (agi_support) {
    auto groupings_node = ep_node.child_by_type(
      ATTRIBUTE_COMMAND_CLASS_ASSOCIATION_SUPPORTED_GROUPINGS);
    if (groupings_node.reported_exists()) {
      zwave_command_class_agi_test_perform_discovery(groupings_node);
      return;
    }
    if (groupings_node.is_valid()) {
      attribute_resolver_set_resolution_listener(
        groupings_node,
        zwave_command_class_agi_test_perform_discovery);
    } else {
      sl_log_warning(LOG_TAG, "supported grouping not yet available");
    }

    discovery_state = mode_discovery_state_t::AGI_TEST_PENDING;
    return;
  }
  discovery_state = mode_discovery_state_t::NOTIFICATION_TEST_PENDING;
}
///////////////////////////////////////////////////////////////////////////////
// Helpers for Pull mode and probe handling
///////////////////////////////////////////////////////////////////////////////
static void pull_mode_check_and_clear_persistence(attribute ep_node,
                                                  const uint8_t *frame_data)
{
  const auto *frame
    = reinterpret_cast<const ZW_NOTIFICATION_REPORT_1BYTE_V3_FRAME *>(
      frame_data);
  auto prev_events = pull_mode_queue.find(ep_node);

  if (prev_events == pull_mode_queue.cend()) {
    std::vector<struct event_queue> queue_entry;
    struct event_queue entry = {frame->notificationType, frame->mevent, 0xFF};
    if ((frame->properties1
         & NOTIFICATION_REPORT_PROPERTIES1_SEQUENCE_BIT_MASK_V3)
        != 0)
      entry.seq = frame->sequenceNumber;
    queue_entry.emplace(queue_entry.end(), entry);
    pull_mode_queue.emplace(std::make_pair(ep_node, queue_entry));
    return;
  }

  // check if current and last event notification have same seq
  if (prev_events->second[0].type == frame->notificationType
      && prev_events->second[0].event == frame->mevent) {
    if (((frame->properties1
          & NOTIFICATION_REPORT_PROPERTIES1_SEQUENCE_BIT_MASK_V3)
           != 0
         && frame->sequenceNumber == prev_events->second[0].seq)
        || (prev_events->second.size() == 4)) {
      auto notification_type_node = ep_node.child_by_type(ATTRIBUTE(TYPE));
      notification_type_node.set_desired<uint8_t>(
        notification_type_node.reported<uint8_t>());
      pull_mode_queue.erase(ep_node);
    } else {
      struct event_queue entry = {frame->notificationType, frame->mevent, 0xFF};
      if ((frame->properties1
           & NOTIFICATION_REPORT_PROPERTIES1_SEQUENCE_BIT_MASK_V3)
          != 0)
        entry.seq = frame->sequenceNumber;
      prev_events->second.insert(prev_events->second.cbegin(), entry);
    }
  } else {
    pull_mode_queue.erase(ep_node);
  }
}

static void pull_mode_trigger_next_probe(attribute ep_node)
{
  auto probe_active = ep_node.child_by_type(ATTRIBUTE(PROBE_ACTIVE));
  // set reported to clear resolver attempt
  probe_active.set_reported<uint8_t>(true);
  // clear reported again to trigger next probe attempt
  probe_active.clear_reported();
}
static void zwave_command_class_notification_trigger_probe()
{
  attribute_store::attribute home_node = get_zpc_network_node();
  try {
    sl_log_debug(LOG_TAG, "periodic probe started for pull nodes");
    for (auto end_node: home_node.children(ATTRIBUTE_NODE_ID)) {
      for (auto ep_node: end_node.children(ATTRIBUTE_ENDPOINT_ID)) {
        // devices which doesn't support Notification CC to be skipped
        if (!ep_node.child_by_type(ATTRIBUTE(VERSION)).is_valid())
          continue;
        // push nodes to be skipped
        if (ep_node.child_by_type_and_value(ATTRIBUTE(MODE), PUSH_MODE)
              .is_valid())
          continue;

        // clear probe active node to start probe process
        pull_mode_trigger_next_probe(ep_node);
      }
    }
  } catch (const std::exception &ex) {
    // continue with next state in case of erorrs
    sl_log_warning(LOG_TAG, "Failed during probe process: %s", ex.what());
  }
}

//////////////////////////////////////////////////////////////////////
static sl_status_t zwave_command_class_notification_update_state_event(
  const attribute &notification_type_node,
  uint8_t state,
  const ZW_NOTIFICATION_REPORT_1BYTE_V3_FRAME *frame,
  uint16_t frame_length)
{
  int32_t notification_event = frame->mevent;
  if (notification_event == 0xFE) {
    notification_event = 0;
  }
  auto version_node
    = notification_type_node.parent().child_by_type(ATTRIBUTE(VERSION));
  zwave_cc_version_t version = version_node.reported<uint8_t>();

  attribute notification_event_node;
  if (version > 2) {
    notification_event_node
      = attribute_store_get_node_child_by_value(notification_type_node,
                                                ATTRIBUTE(STATE),
                                                REPORTED_ATTRIBUTE,
                                                &state,
                                                sizeof(state),
                                                0);
    notification_event_node
      = notification_event_node.child_by_type(ATTRIBUTE(EVENT), 0);
    sl_log_info(NOTIFICATION_TAG,
                "<Type: %u> State: %u Event: %u",
                notification_type_node.reported<uint8_t>(),
                state,
                notification_event);
  } else {
    // version 1 and 2 do not have a concept of state hence it will be a dummy parent node
    auto state_node = notification_type_node.child_by_type(ATTRIBUTE(STATE));
    notification_event_node = state_node.emplace_node(ATTRIBUTE(EVENT));
    sl_log_info(NOTIFICATION_TAG,
                "<zwAlarm Type: %u> Event: %u",
                notification_type_node.reported<uint8_t>(),
                notification_event);
  }

  if (!notification_event_node.is_valid()) {
    sl_log_debug(LOG_TAG,
                 "Failed to find the State -> Event node under Notification "
                 "Type Attribute %d, for State value %d",
                 notification_type_node,
                 state);
    return SL_STATUS_OK;
  }
  notification_event_node.set_reported(notification_event);

  uint8_t state_event_param_len
    = frame->properties1
      & NOTIFICATION_REPORT_PROPERTIES1_EVENT_PARAMETERS_LENGTH_MASK_V4;
  attribute notification_event_param
    = notification_event_node.child_by_type(ATTRIBUTE(EVENT_PARAMETERS), 0);
  if (!notification_event_param.is_valid()) {
    sl_log_debug(
      LOG_TAG,
      "Failed to find the Event Parameter attribute under attribute %d",
      notification_event_node);
    return SL_STATUS_OK;
  }
  if (state_event_param_len > 0
      && frame_length >= (NOTIFICATION_REPORT_EVENT_STATE_PARAMETER_OFFSET
                          + state_event_param_len)) {
    // Update the parameters based on the size, also if the event parameter
    // Length (state_event_param_len) is zero (attribute back to undefined)
    int32_t state_event = frame->eventParameter1;
    notification_event_param.set_reported(state_event);
    sl_log_info(NOTIFICATION_TAG,
                "Event: %u Param: %u",
                notification_event,
                state_event);
  } else {
    // No Event/state parameter byte added to the payload, we just undefine the
    // value again.
    notification_event_param.clear_reported();
  }
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_notification_report_cmd_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  if (frame_length < 3) {
    return SL_STATUS_OK;
  }

  const auto *frame
    = reinterpret_cast<const ZW_NOTIFICATION_REPORT_1BYTE_V3_FRAME *>(
      frame_data);

  // Get the unid of the sending node
  unid_t sending_node_unid;
  zwave_unid_from_node_id(connection_info->remote.node_id, sending_node_unid);

  // Get the corresponding attribute store id for the sending node endpoint
  try {
    attribute ep_node = attribute_store_network_helper_get_endpoint_node(
      sending_node_unid,
      connection_info->remote.endpoint_id);
    auto mode_node = ep_node.child_by_type(ATTRIBUTE(MODE));
    uint8_t mode   = PUSH_MODE;

    if (discovery_state == mode_discovery_state_t::NOTIFICATION_TEST_PENDING) {
      if (frame->notificationStatus == PUSH_REPORT_STATUS_ENABLED) {
        mode            = PUSH_MODE;
        discovery_state = mode_discovery_state_t::PUSH_MODE_DETECTED;
      } else {
        mode            = PULL_MODE;
        discovery_state = mode_discovery_state_t::PULL_MODE_DETECTED;
        ep_node.emplace_node(ATTRIBUTE(PROBE_ACTIVE));
      }
      mode_node.set_reported(mode);
    } else {
      mode = mode_node.reported<uint8_t>();
    }

    // Handling V1 Alarm Type and Level, they are proprietary Type and Level.
    // The values are usually specified in the product manual.
    // If the V1 Alarm Type is not supported, these fields MUST be set to 0x00.
    // ATTRIBUTE_COMMAND_CLASS_NOTIFICATION_V1_ALARM_TYPE should be added under
    // endpoint attribute and the level shall be under TYPE attribute:
    // EP --> ..V1_ALARAM_TYPE-->..V1_ALARM_LEVEL
    const uint8_t v1_alarm_type = frame->v1AlarmType;
    attribute v1_alarm_type_node;
    if (v1_alarm_type != 0) {
      v1_alarm_type_node
        = attribute_store_get_node_child_by_value(ep_node,
                                                  ATTRIBUTE(V1_ALARM_TYPE),
                                                  REPORTED_ATTRIBUTE,
                                                  &v1_alarm_type,
                                                  sizeof(v1_alarm_type),
                                                  0);
      if (v1_alarm_type_node.is_valid()) {
        v1_alarm_type_node.child_by_type(ATTRIBUTE(V1_ALARM_LEVEL))
          .set_reported(frame->v1AlarmLevel);
      } else {
        v1_alarm_type_node = ep_node.add_node(ATTRIBUTE(V1_ALARM_TYPE));
        v1_alarm_type_node.set_reported(v1_alarm_type);
        attribute vl_alarm_level_node
          = v1_alarm_type_node.add_node(ATTRIBUTE(V1_ALARM_LEVEL));
        vl_alarm_level_node.set_reported(frame->v1AlarmLevel);
      }
      sl_log_info(NOTIFICATION_TAG,
                  "<V1 Type: %u> level: %u",
                  v1_alarm_type,
                  frame->v1AlarmLevel);
    }

    if (mode == PULL_MODE) {
      if (frame->notificationStatus == PULL_REPORT_STATUS_EMPTY) {
        sl_log_debug(LOG_TAG,
                     "Pull sensor queue empty, stopped further probing");
        auto probe_active = ep_node.child_by_type(ATTRIBUTE(PROBE_ACTIVE));
        probe_active.set_reported<uint8_t>(false);
        return SL_STATUS_OK;
      }
      pull_mode_check_and_clear_persistence(ep_node, frame_data);
    }

    const uint8_t notification_type = frame->notificationType;
    attribute notification_type_node
      = attribute_store_get_node_child_by_value(ep_node,
                                                ATTRIBUTE(TYPE),
                                                REPORTED_ATTRIBUTE,
                                                &notification_type,
                                                sizeof(notification_type),
                                                0);
    if (!notification_type_node.is_valid()) {
      // Unknown/Unsupported Type needs to be reported to user
      sl_log_info(NOTIFICATION_TAG,
                  "Unknown Type: %u State/Event: %u",
                  frame->notificationType,
                  frame->mevent);
      if (mode == PULL_MODE)
        pull_mode_trigger_next_probe(ep_node);
      return SL_STATUS_OK;
    }

    uint8_t event = frame->mevent;
    // If event is 0 it we find the event to use for state lookup in the event
    // parameters, as due to spec the "IDLE" shall have the event to set to idle
    // as paramter.
    if (((event == 0)
         && (frame->properties1
             & NOTIFICATION_REPORT_PROPERTIES1_EVENT_PARAMETERS_LENGTH_MASK_V8)
              == 0)
        || (event == 0xFE)) {
      sl_log_debug(LOG_TAG,
                   "Notification Idle Event without any event parameters or "
                   "Event 0xFE received. Setting all states to idle.");
      std::set<unsigned int> updated_states;
      for (auto const &elem:
           notification_event_state_map.at(frame->notificationType)) {
        try {
          if (updated_states.find(elem.second) != updated_states.end()) {
            // State is already set to idle
            continue;
          }
          updated_states.insert(elem.second);
          zwave_command_class_notification_update_state_event(
            notification_type_node,
            elem.second,
            frame,
            frame_length);
        } catch (const std::exception &ex) {
          // continue with next state in case of erorrs
          sl_log_warning(
            LOG_TAG,
            "Failed to write Notification Event to attribute store: %s",
            ex.what());
        }
      }
      if (mode == PULL_MODE)
        pull_mode_trigger_next_probe(ep_node);

      // We always return OK here, even for errors
      return SL_STATUS_OK;
    }

    if (event == 0) {
      event = frame->eventParameter1;
    }

    uint8_t state;
    try {
      // Lookup if the event is a state change
      state = static_cast<uint8_t>(
        notification_event_state_map.at(frame->notificationType).at(event));
    } catch (const std::exception &e) {
      // If no state is looked up it is an event, write to LAST_EVENT attribute
      state = NOTIFICATION_STATE_LAST_EVENT;
    }
    zwave_command_class_notification_update_state_event(notification_type_node,
                                                        state,
                                                        frame,
                                                        frame_length);
    if (mode == PULL_MODE)
      pull_mode_trigger_next_probe(ep_node);

  } catch (const std::exception &ex) {
    // do nothing
    sl_log_warning(LOG_TAG, "%s", ex.what());
  }
  return SL_STATUS_OK;
}

static sl_status_t
  zwave_command_class_supported_notification_types_report_cmd_handler(
    const zwave_controller_connection_info_t *connection_info,
    const uint8_t *frame_data,
    uint16_t frame_length)
{
  if (frame_length < 3) {
    return SL_STATUS_OK;
  }

  // Get the unid of the sending node
  unid_t sending_node_unid;
  zwave_unid_from_node_id(connection_info->remote.node_id, sending_node_unid);
  // Get the corresponding attribute store id for the sending node endpoint
  attribute_store_node_t ep_node
    = attribute_store_network_helper_get_endpoint_node(
      sending_node_unid,
      connection_info->remote.endpoint_id);

  zwave_cc_version_t version
    = zwave_node_get_command_class_version(COMMAND_CLASS_NOTIFICATION_V8,
                                           connection_info->remote.node_id,
                                           connection_info->remote.endpoint_id);
  // Extracting supported notification types for Notification_CC_V4
  uint32_t notification_types_bits               = 0x0000;
  uint32_t notification_type_mask                = 0x0000;
  uint8_t number_of_supported_notification_types = 0;
  uint8_t supported_notification_types[MAX_SUPPORTED_NOTIFICATION_TYPES];
  uint8_t number_of_bit_masks
    = frame_data[2]
      & NOTIFICATION_SUPPORTED_REPORT_PROPERTIES1_NUMBER_OF_BIT_MASKS_MASK_V4;

  if (number_of_bit_masks == 0 || (number_of_bit_masks > 3)) {
    sl_log_warning(
      LOG_TAG,
      "Supported notification types Bit Masks length is not supported\n");
  } else {
    if (number_of_bit_masks == 3) {
      notification_types_bits = frame_data[5];
    }
    if (number_of_bit_masks >= 2) {
      notification_types_bits = (notification_types_bits << 8) | frame_data[4];
    }
    if (number_of_bit_masks >= 1) {
      notification_types_bits = (notification_types_bits << 8) | frame_data[3];
    }
  }

  for (size_t i = 1; i <= MAX_SUPPORTED_NOTIFICATION_TYPES; i++) {
    notification_type_mask = 1 << i;
    notification_type_mask &= notification_types_bits;
    if (notification_type_mask) {
      switch (notification_type_mask) {
        case 0x0002:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_SMOKE_V4;
          break;
        case 0x0004:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_CO_V4;
          break;
        case 0x0008:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_CO2_V4;
          break;
        case 0x0010:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_HEAT_V4;
          break;
        case 0x0020:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_WATER_V4;
          break;
        case 0x0040:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_ACCESS_CONTROL_V4;
          break;
        case 0x0080:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_HOME_SECURITY_V4;
          break;
        case 0x0100:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_POWER_MANAGEMENT_V4;
          break;
        case 0x0200:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_SYSTEM_V4;
          break;
        case 0x0400:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_EMERGENCY_V4;
          break;
        case 0x0800:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_CLOCK_V4;
          break;
        case 0x1000:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_APPLIANCE_V4;
          break;
        case 0x2000:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_REPORT_HOME_HEALTH_V4;
          break;
        case 0x4000:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_GET_SIREN_V6;
          break;
        case 0x8000:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_GET_WATER_VALVE_V7;
          break;
        case 0x10000:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_GET_WEATHER_ALARM_V8;
          break;
        case 0x20000:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_GET_IRRIGATION_V8;
          break;
        case 0x40000:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_GET_GAS_ALARM_V8;
          break;
        case 0x80000:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_GET_PEST_CONTROL_V8;
          break;
        case 0x100000:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_GET_LIGHT_SENSOR_V8;
          break;
        case 0x200000:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_GET_WATER_QUALITY_MONITORING_V8;
          break;
        case 0x400000:
          supported_notification_types[number_of_supported_notification_types]
            = NOTIFICATION_GET_HOME_MONITORING_V8;
          break;
      }
      number_of_supported_notification_types++;
    }
  }

  // Update the notification types reported values in the Attribute Store
  attribute_store_node_t supported_notification_types_node
    = attribute_store_get_first_child_by_type(
      ep_node,
      ATTRIBUTE(SUPPORTED_NOTIFICATION_TYPES));
  attribute_store_set_node_attribute_value(
    supported_notification_types_node,
    REPORTED_ATTRIBUTE,
    supported_notification_types,
    number_of_supported_notification_types);

  for (uint8_t i = 0; i < number_of_supported_notification_types; i++) {
    attribute_store_node_t notification_type
      = attribute_store_get_node_child_by_value(
        ep_node,
        ATTRIBUTE(TYPE),
        REPORTED_ATTRIBUTE,
        &supported_notification_types[i],
        sizeof(supported_notification_types[i]),
        0);
    if (notification_type != ATTRIBUTE_STORE_INVALID_NODE) {
      sl_log_debug(LOG_TAG,
                   "Notification type[%u] already in attribute store",
                   supported_notification_types[i]);
      continue;
    }
    notification_type = attribute_store_add_node(ATTRIBUTE(TYPE), ep_node);

    attribute_store_set_node_attribute_value(
      notification_type,
      REPORTED_ATTRIBUTE,
      &supported_notification_types[i],
      sizeof(supported_notification_types[i]));
    if (version > 2) {
      auto node
        = attribute_store_add_node(ATTRIBUTE(SUPPORTED_STATES_OR_EVENTS),
                                   notification_type);
      /* Wait till last type is completely resolved, to discover pull/push mode*/
      if ((i == (number_of_supported_notification_types - 1))
          && (discovery_state
              == mode_discovery_state_t::NOTIFICATION_TEST_PENDING)) {
        attribute_resolver_set_resolution_listener(
          node,
          zwave_command_class_continue_mode_discovery);
      }
    } else if (version > 1) {
      attribute_store_add_node(ATTRIBUTE(STATE), notification_type);
    }
  }

  return SL_STATUS_OK;
}

static sl_status_t
  zwave_command_class_supported_state_event_types_report_cmd_handler(
    const zwave_controller_connection_info_t *connection_info,
    const uint8_t *frame_data,
    uint16_t frame_length)
{
  if (frame_length < 4) {
    sl_log_debug(LOG_TAG,
                 "Received an Event Supported Report Command with "
                 "too small frame length (%d). Ignoring",
                 frame_length);
    return SL_STATUS_OK;
  }

  const auto *report_frame
    = reinterpret_cast<const ZW_EVENT_SUPPORTED_REPORT_1BYTE_V3_FRAME *>(
      frame_data);
  unsigned int number_of_bytes
    = (report_frame->properties1
       & EVENT_SUPPORTED_REPORT_PROPERTIES1_NUMBER_OF_BIT_MASKS_MASK_V3);
  if (number_of_bytes == 0) {
    sl_log_debug(LOG_TAG,
                 "The Notification Type is not supported. It seems that we "
                 "sent a wrong Get Command or that the supporting node "
                 "did not report correct data.");
    return SL_STATUS_OK;
  }
  // Get the unid of the sending node
  unid_t sending_node_unid;
  zwave_unid_from_node_id(connection_info->remote.node_id, sending_node_unid);
  // Get the corresponding attribute store id for the sending node endpoint
  attribute_store_node_t ep_node
    = attribute_store_network_helper_get_endpoint_node(
      sending_node_unid,
      connection_info->remote.endpoint_id);
  if (ATTRIBUTE_STORE_INVALID_NODE == ep_node) {
    sl_log_error(LOG_TAG,
                 "Failed to lookup Endpoint (%d)",
                 connection_info->remote.endpoint_id);
    return SL_STATUS_OK;
  }

  attribute_store_node_t notification_node
    = attribute_store_get_node_child_by_value(
      ep_node,
      ATTRIBUTE(TYPE),
      REPORTED_ATTRIBUTE,
      &report_frame->notificationType,
      sizeof(report_frame->notificationType),
      0);
  if (ATTRIBUTE_STORE_INVALID_NODE == notification_node) {
    sl_log_debug(LOG_TAG,
                 "Failed to lookup Notification Type (%d) under Endpoint (%d)",
                 report_frame->notificationType,
                 connection_info->remote.endpoint_id);
    return SL_STATUS_OK;
  }

  attribute_store_node_t supported_state_or_event_node
    = attribute_store_get_first_child_by_type(
      notification_node,
      ATTRIBUTE(SUPPORTED_STATES_OR_EVENTS));
  if (ATTRIBUTE_STORE_INVALID_NODE == supported_state_or_event_node) {
    sl_log_debug(LOG_TAG,
                 "Failed to lookup Notification Supported Event State "
                 "Attribute under Notification Type (%d)",
                 report_frame->notificationType);
    return SL_STATUS_OK;
  }

  std::vector<uint8_t> supported_events;
  bool create_last_event_attribute = false;
  // Here we loop over all bits in the bitmask and lookup if the event is
  // actually a state change. If it's a state change we create an attribute for
  // the state.
  for (size_t byte_idx = 0; byte_idx < number_of_bytes; byte_idx++) {
    const uint8_t byte = (&(report_frame->bitMask1))[byte_idx];
    for (size_t bit_idx = 0; bit_idx < 8; bit_idx++) {
      if (((byte >> bit_idx) & 1) == 1) {
        const uint8_t event = byte_idx * 8 + bit_idx;
        supported_events.push_back(event);
        try {
          // Lookup if the event is a state change
          uint8_t state
            = notification_event_state_map.at(report_frame->notificationType)
                .at(event);
          // Lookup if the state node is already created in the attribute store
          attribute_store_node_t notification_state_node
            = attribute_store_get_node_child_by_value(notification_node,
                                                      ATTRIBUTE(STATE),
                                                      REPORTED_ATTRIBUTE,
                                                      &state,
                                                      sizeof(state),
                                                      0);
          if (notification_state_node != ATTRIBUTE_STORE_INVALID_NODE) {
            // Node already exists, continue with next bit
            continue;
          }
          notification_state_node
            = attribute_store_add_node(ATTRIBUTE(STATE), notification_node);
          if (notification_state_node == ATTRIBUTE_STORE_INVALID_NODE) {
            sl_log_error(LOG_TAG,
                         "Failed to create attribute for the Notification "
                         "State under attribute %d.",
                         notification_state_node);
            assert(false);
            continue;
          }
          const attribute_store_node_t event_type
            = attribute_store_add_node(ATTRIBUTE(EVENT),
                                       notification_state_node);
          if (event_type == ATTRIBUTE_STORE_INVALID_NODE) {
            sl_log_error(LOG_TAG,
                         "Failed to create attribute for the Notification "
                         "Event Type under attribute %d.",
                         notification_state_node);
            assert(false);
            continue;
          }
          if (ATTRIBUTE_STORE_INVALID_NODE
              == attribute_store_add_node(ATTRIBUTE(EVENT_PARAMETERS),
                                          event_type)) {
            sl_log_error(LOG_TAG,
                         "Failed to create attribute for the Notification "
                         "Event Parameters under attribute %d.",
                         event_type);
            assert(false);
            continue;
          }
          attribute_store_set_node_attribute_value(notification_state_node,
                                                   REPORTED_ATTRIBUTE,
                                                   &state,
                                                   sizeof(state));
        } catch (const std::exception &e) {
          // Event is not a state change, this is okay just continue.
          // Set that last_event attribute shall be created
          create_last_event_attribute = true;
        }
      }
    }
  }
  // Store supported events as array in attribute store
  attribute_store_set_node_attribute_value(supported_state_or_event_node,
                                           REPORTED_ATTRIBUTE,
                                           supported_events.data(),
                                           supported_events.size());
  if (create_last_event_attribute) {
    // Create attribute for last received event, if any non-state events exist
    // for the notification
    uint8_t state = NOTIFICATION_STATE_LAST_EVENT;
    // Lookup if the state node is already created in the attribute store
    attribute_store_node_t notification_state_node
      = attribute_store_get_node_child_by_value(notification_node,
                                                ATTRIBUTE(STATE),
                                                REPORTED_ATTRIBUTE,
                                                &state,
                                                sizeof(state),
                                                0);
    if (notification_state_node != ATTRIBUTE_STORE_INVALID_NODE) {
      // Node already exists, continue with next bit
    } else {
      notification_state_node
        = attribute_store_add_node(ATTRIBUTE(STATE), notification_node);
      if (notification_state_node == ATTRIBUTE_STORE_INVALID_NODE) {
        sl_log_error(LOG_TAG,
                     "Failed to create attribute for the Notification "
                     "State under attribute %d.",
                     notification_state_node);
        assert(false);
      }
      attribute_store_set_node_attribute_value(notification_state_node,
                                               REPORTED_ATTRIBUTE,
                                               &state,
                                               sizeof(state));
    }
  }

  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_notification_control_handler(
  const zwave_controller_connection_info_t *connection_info,
  const uint8_t *frame_data,
  uint16_t frame_length)
{
  if (frame_length <= COMMAND_INDEX) {
    return SL_STATUS_FAIL;
  }
  if (frame_data[COMMAND_CLASS_INDEX] != COMMAND_CLASS_NOTIFICATION_V8) {
    return SL_STATUS_NOT_SUPPORTED;
  }

  switch (frame_data[COMMAND_INDEX]) {
    case NOTIFICATION_REPORT_V4:
      return zwave_command_class_notification_report_cmd_handler(
        connection_info,
        frame_data,
        frame_length);

    case NOTIFICATION_SUPPORTED_REPORT_V4:
      return zwave_command_class_supported_notification_types_report_cmd_handler(
        connection_info,
        frame_data,
        frame_length);

    case EVENT_SUPPORTED_REPORT_V4:
      return zwave_command_class_supported_state_event_types_report_cmd_handler(
        connection_info,
        frame_data,
        frame_length);

    default:
      return SL_STATUS_NOT_SUPPORTED;
  }
}

/**
 * @brief A function that detects the support of the Notification CC and
 * creates the required Attributes for the Notification Command Class
 * sub-component.
 *
 * @param updated_node The Attribute Store node that just got updated.
 * @param change       The Attribute Store change that the node underwent.
 */
void zwave_command_class_notification_on_version_attribute_update(
  attribute_store_node_t updated_node, attribute_store_change_t change)
{
  if (change == ATTRIBUTE_DELETED || change == ATTRIBUTE_CREATED) {
    return;
  }

  if (is_zwave_command_class_filtered_for_root_device(
        COMMAND_CLASS_NOTIFICATION_V8,
        updated_node)
      == true) {
    return;
  }
  // Check that we have the right type of attribute.
  assert(ATTRIBUTE(VERSION) == attribute_store_get_node_type(updated_node));

  zwave_node_id_t node_id         = 0;
  zwave_endpoint_id_t endpoint_id = 0;

  if (SL_STATUS_OK
      != attribute_store_network_helper_get_zwave_ids_from_node(updated_node,
                                                                &node_id,
                                                                &endpoint_id)) {
    assert(false && "Should always be able to get command class version here");
    return;
  }
  zwave_cc_version_t version
    = zwave_node_get_command_class_version(COMMAND_CLASS_NOTIFICATION_V8,
                                           node_id,
                                           endpoint_id);

  // Here we know that a node supported Notification Command Class.
  attribute_store_node_t parent_node
    = attribute_store_get_node_parent(updated_node);
  // For version 1 Notification Supported Get is not available
  if (version > 1) {
    // Verify if there is already supported notification types attribute.
    // Note that updated_node is the ENDPOINT node in the Attribute Store.
    attribute_store_node_t supported_notification_type_node
      = attribute_store_get_first_child_by_type(
        parent_node,
        ATTRIBUTE(SUPPORTED_NOTIFICATION_TYPES));
    // If not, create it
    if (supported_notification_type_node == ATTRIBUTE_STORE_INVALID_NODE) {
      attribute_store_add_node(ATTRIBUTE(SUPPORTED_NOTIFICATION_TYPES),
                               parent_node);
    }
  }
  attribute_store_node_t notification_mode_node
    = attribute_store_get_first_child_by_type(parent_node, ATTRIBUTE(MODE));
  if (notification_mode_node == ATTRIBUTE_STORE_INVALID_NODE) {
    attribute_store_add_node(ATTRIBUTE(MODE), parent_node);
    zwave_command_class_notification_pull_push_discovery(node_id, endpoint_id);
  }
}

static sl_status_t zwave_command_class_alarm_get(attribute_store_node_t node,
                                                 uint8_t *frame,
                                                 uint16_t *frame_len)
{
  auto alarm_get_frame = reinterpret_cast<ZW_ALARM_GET_V2_FRAME *>(frame);

  attribute_store::attribute ep_node
    = attribute_store_get_first_parent_with_type(node, ATTRIBUTE_ENDPOINT_ID);
  auto version_node          = ep_node.child_by_type(ATTRIBUTE(VERSION));
  zwave_cc_version_t version = version_node.reported<zwave_cc_version_t>();

  if (version > 2) {
    sl_log_error(LOG_TAG, "Should not be called for versions > v2");
    return SL_STATUS_FAIL;
  }
  alarm_get_frame->cmdClass  = COMMAND_CLASS_NOTIFICATION_V8;
  alarm_get_frame->cmd       = NOTIFICATION_GET_V8;
  alarm_get_frame->alarmType = 0x00;  //v1 AlarmType
  // read the Notification type
  attribute_store_node_t type_node
    = attribute_store_get_first_parent_with_type(node, ATTRIBUTE(TYPE));
  uint8_t notification_type;
  attribute_store_get_reported(type_node,
                               &notification_type,
                               sizeof(notification_type));
  alarm_get_frame->zwaveAlarmType = notification_type;

  *frame_len = sizeof(ZW_ALARM_GET_V2_FRAME);

  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_notification_set(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_len)
{
  attribute_store::attribute type_node(node);
  try {
    auto type      = type_node.desired<uint8_t>();
    auto mode_node = type_node.parent().child_by_type(ATTRIBUTE(MODE));
    frame_generator.initialize_frame(NOTIFICATION_SET_V8,
                                     frame,
                                     sizeof(ZW_NOTIFICATION_SET_V8_FRAME));
    frame_generator.add_raw_byte(type);
    // if mode is not yet determined, send 0xFF for notification test
    if (!mode_node.reported_exists()) {
      frame_generator.add_raw_byte(0xFF);
    }
    // if pull mode, then send 0x00 to clear persistent notification
    else if (mode_node.reported<uint8_t>() == PULL_MODE) {
      frame_generator.add_raw_byte(0x00);
    }
    frame_generator.validate_frame(frame_len);

    // Only in case of notification test for mode discovery
    if (!mode_node.reported_exists()) {
      auto event_node = type_node.child_by_type(ATTRIBUTE(STATE))
                          .child_by_type(ATTRIBUTE(EVENT));
      event_node.clear_reported();
    }
  } catch (const std::exception &e) {
    sl_log_error(
      LOG_TAG,
      "Error while generating Notification Set Association Set frame : %s",
      e.what());
    return SL_STATUS_NOT_SUPPORTED;
  }
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_probe_notification(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_len)
{
  auto notification_get_frame
    = reinterpret_cast<ZW_NOTIFICATION_GET_V4_FRAME *>(frame);

  auto ep_node = attribute(node).first_parent(ATTRIBUTE_ENDPOINT_ID);
  uint8_t mode = ep_node.child_by_type(ATTRIBUTE(MODE)).reported<uint8_t>();

  if (mode != PULL_MODE) {
    sl_log_warning(LOG_TAG, "Ignoring probe attempted for push sensor");
    return SL_STATUS_FAIL;
  }

  notification_get_frame->cmdClass         = COMMAND_CLASS_NOTIFICATION_V8;
  notification_get_frame->cmd              = NOTIFICATION_GET_V8;
  notification_get_frame->v1AlarmType      = 0x00;
  notification_get_frame->notificationType = 0xFF;
  notification_get_frame->mevent           = 0x00;

  *frame_len = sizeof(ZW_NOTIFICATION_GET_V4_FRAME);
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_notification_get(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_len)
{
  auto notification_get_frame
    = reinterpret_cast<ZW_NOTIFICATION_GET_V4_FRAME *>(frame);
  notification_get_frame->cmdClass    = COMMAND_CLASS_NOTIFICATION_V8;
  notification_get_frame->cmd         = NOTIFICATION_GET_V8;
  notification_get_frame->v1AlarmType = 0x00;
  // read the Notification type
  attribute_store_node_t type_node
    = attribute_store_get_first_parent_with_type(node, ATTRIBUTE(TYPE));
  uint8_t notification_type;
  attribute_store_get_reported(type_node,
                               &notification_type,
                               sizeof(notification_type));
  notification_get_frame->notificationType = notification_type;

  // Get Notification Event / State from attribute store
  uint8_t notification_state;
  attribute_store_get_reported(
    attribute_store_get_first_parent_with_type(node, ATTRIBUTE(STATE)),
    &notification_state,
    sizeof(notification_state));
  uint8_t supported_notification_states[MAX_SUPPORTED_NOTIFICATION_STATES];
  uint8_t number_of_supported_states;
  attribute_store_node_t supported_states_node
    = attribute_store_get_first_child_by_type(
      type_node,
      ATTRIBUTE(SUPPORTED_STATES_OR_EVENTS));
  attribute_store_get_node_attribute_value(supported_states_node,
                                           REPORTED_ATTRIBUTE,
                                           supported_notification_states,
                                           &number_of_supported_states);
  if (!number_of_supported_states) {
    notification_get_frame->mevent = 0x00;
  } else {
    notification_get_frame->mevent
      = supported_notification_states[notification_state];
  }
  *frame_len = sizeof(ZW_NOTIFICATION_GET_V4_FRAME);

  // Just in case the supporting node does not answer Notification
  // Get commands, we set the reported value of all events for the type to 0
  // to prevent more resolutions.
  const int32_t notification_event = 0;
  attribute_store_set_all_children_reported(type_node,
                                            ATTRIBUTE(EVENT),
                                            &notification_event,
                                            sizeof(notification_event));

  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_supported_event_get(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_len)
{
  ZW_EVENT_SUPPORTED_GET_V4_FRAME *supported_event_get_frame
    = (ZW_EVENT_SUPPORTED_GET_V4_FRAME *)frame;
  supported_event_get_frame->cmdClass = COMMAND_CLASS_NOTIFICATION_V8;
  supported_event_get_frame->cmd      = EVENT_SUPPORTED_GET_V8;
  // read the Notification type
  uint8_t notification_type;
  attribute_store_read_value(attribute_store_get_node_parent(node),
                             REPORTED_ATTRIBUTE,
                             &notification_type,
                             sizeof(notification_type));
  supported_event_get_frame->notificationType = notification_type;
  *frame_len = sizeof(ZW_EVENT_SUPPORTED_GET_V4_FRAME);
  return SL_STATUS_OK;
}

static sl_status_t zwave_command_class_supported_notification_types_get(
  attribute_store_node_t node, uint8_t *frame, uint16_t *frame_len)
{
  ZW_NOTIFICATION_SUPPORTED_GET_V4_FRAME *supported_notification_get_frame
    = (ZW_NOTIFICATION_SUPPORTED_GET_V4_FRAME *)frame;
  supported_notification_get_frame->cmdClass = COMMAND_CLASS_NOTIFICATION_V8;
  supported_notification_get_frame->cmd      = NOTIFICATION_SUPPORTED_GET_V8;
  *frame_len = sizeof(ZW_NOTIFICATION_SUPPORTED_GET_V4_FRAME);
  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions
///////////////////////////////////////////////////////////////////////////////

sl_status_t zwave_command_class_notification_init()
{
  attribute_resolver_register_rule(
    ATTRIBUTE(SUPPORTED_NOTIFICATION_TYPES),
    nullptr,
    zwave_command_class_supported_notification_types_get);
  attribute_resolver_register_rule(ATTRIBUTE(STATE),
                                   nullptr,
                                   zwave_command_class_alarm_get);
  attribute_resolver_register_rule(ATTRIBUTE(TYPE),
                                   zwave_command_class_notification_set,
                                   nullptr);
  attribute_resolver_register_rule(ATTRIBUTE(EVENT),
                                   nullptr,
                                   zwave_command_class_notification_get);
  attribute_resolver_register_rule(ATTRIBUTE(PROBE_ACTIVE),
                                   nullptr,
                                   zwave_command_class_probe_notification);

  // Supported event reported handler is not implemented so we do not use the rule for now,
  // register the rule when the report handler is implemented.
  attribute_resolver_register_rule(ATTRIBUTE(SUPPORTED_STATES_OR_EVENTS),
                                   nullptr,
                                   zwave_command_class_supported_event_get);

  // Here the module is listening for the creation of
  // ATTRIBUTE_COMMAND_CLASS_NOTIFICATION_VERSION attribute.
  attribute_store_register_callback_by_type(
    zwave_command_class_notification_on_version_attribute_update,
    ATTRIBUTE(VERSION));

  // Tell AGI that we want to receive Notification Reports
  // via assocation groups
  zwave_command_class_agi_request_to_establish_association(
    COMMAND_CLASS_NOTIFICATION_V8,
    NOTIFICATION_REPORT_V8);

  // Register Notification CC handler to the Z-Wave CC framework
  zwave_command_handler_t handler = {};
  handler.support_handler         = nullptr;
  handler.control_handler = &zwave_command_class_notification_control_handler;
  handler.minimal_scheme  = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  handler.command_class   = COMMAND_CLASS_NOTIFICATION_V8;
  handler.version         = NOTIFICATION_VERSION_V8;
  handler.manual_security_validation = false;
  handler.command_class_name         = "Notification";
  handler.comments                   = "";

  zwave_command_handler_register_handler(handler);

  process_start(&zwave_command_class_notification_process, NULL);

  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Contiki process to handle probing
//////////////////////////////////////////////////////////////////////////////
PROCESS_THREAD(zwave_command_class_notification_process, ev, data)
{
  PROCESS_BEGIN();
  while (1) {
    switch (ev) {
      case PROCESS_EVENT_INIT:
        etimer_set(&notification_probe_timer, PULL_NODE_PROBE_INTERVAL);
        break;
      case PROCESS_EVENT_EXIT:
        etimer_stop(&notification_probe_timer);
        break;
      case PROCESS_EVENT_TIMER:
        if (data == &notification_probe_timer) {
          zwave_command_class_notification_trigger_probe();
          etimer_set(&notification_probe_timer, PULL_NODE_PROBE_INTERVAL);
        }
        break;
      default:
        break;
    }
    PROCESS_WAIT_EVENT();
  }
  PROCESS_END()
}