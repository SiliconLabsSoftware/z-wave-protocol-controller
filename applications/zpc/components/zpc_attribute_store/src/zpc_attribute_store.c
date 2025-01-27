/* <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "zpc_attribute_store.h"
#include "zpc_attribute_store_network_helper.h"
#include "unify_dotdot_attribute_store_node_state.h"
#include "attribute_store_defined_attribute_types.h"
#include "zpc_attribute_store_type_registration.h"
#include "zpc_attribute_store_register_default_attribute_type_data.h"
#include "unify_dotdot_defined_attribute_types.h"
// Generic includes
#include <string.h>

// Interfaces


// Includes from ZPC components
#include "zwave_network_management.h"

// Unify Components
#include "attribute_store_helper.h"
#include "attribute_store_configuration.h"
#include "unify_dotdot_attribute_store.h"

#include "sl_log.h"
#define LOG_TAG "zpc_attribute_store"

// ZPC Configuration for the Unify DotDot Attribute Store
const unify_dotdot_attribute_store_configuration_t zpc_configuration = {
  .get_endpoint_node_function
  = &attribute_store_network_helper_get_endpoint_node,
  .get_unid_endpoint_function
  = &attribute_store_network_helper_get_unid_endpoint_from_node,
  .get_unid_function = &attribute_store_network_helper_get_unid_from_node,
  .update_attribute_desired_values_on_commands = true,
  .clear_reported_on_desired_updates           = false,
  .automatic_deduction_of_supported_commands   = true,
  .force_read_attributes_enabled               = true,
  .write_attributes_enabled                    = true,
  .publish_desired_attribute_values_to_mqtt    = true,
  .publish_reported_attribute_values_to_mqtt   = true,
  .node_type                                   = ATTRIBUTE_NODE_ID,
  .endpoint_type                               = ATTRIBUTE_ENDPOINT_ID,
};

////////////////////////////////////////////////////////////////////////////////
// Private functions
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Finds all NodeIDs network statuses with one value and change them
 *        to a new one.
 *
 * Network status nodes will be updated only if they currently have the "old_value"
 * reported value.
 *
 * @param home_id_node  The attribute Store node for the HomeID.
 * @param old_value     Network status previous value
 * @param new_value     Network status new value
 *
 */
static void zpc_attribute_store_update_all_network_statuses(
  attribute_store_node_t home_id_node,
  NodeStateNetworkStatus old_value,
  NodeStateNetworkStatus new_value)
{
  // Find all NodeIDs in that network
  uint32_t node_id_index = 0;
  attribute_store_node_t node_id_node
    = attribute_store_get_node_child_by_type(home_id_node,
                                             ATTRIBUTE_NODE_ID,
                                             node_id_index);
  node_id_index += 1;

  while (node_id_node != ATTRIBUTE_STORE_INVALID_NODE) {
    // Transition the network status from old value to new value:
    attribute_store_node_t network_status_node
      = attribute_store_get_node_child_by_value(node_id_node,
                                                DOTDOT_ATTRIBUTE_ID_STATE_NETWORK_STATUS,
                                                REPORTED_ATTRIBUTE,
                                                (uint8_t*)(&old_value),
                                                sizeof(old_value),
                                                0);
    attribute_store_set_reported(network_status_node,
                                 &new_value,
                                 sizeof(new_value));
    // Increment to the next NodeID.
    node_id_node = attribute_store_get_node_child_by_type(home_id_node,
                                                          ATTRIBUTE_NODE_ID,
                                                          node_id_index);
    node_id_index += 1;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////////
bool is_zpc_unid(const unid_t unid)
{
  zwave_node_id_t my_node_id = zwave_network_management_get_node_id();
  unid_t my_unid;
  zwave_unid_from_node_id(my_node_id, my_unid);

  if (strcmp(unid, my_unid) == 0) {
    return true;
  } else {
    return false;
  }
}

attribute_store_node_t get_zpc_network_node()
{
  zwave_home_id_t my_home_id = zwave_network_management_get_home_id();

  return attribute_store_get_node_child_by_value(attribute_store_get_root(),
                                                 ATTRIBUTE_HOME_ID,
                                                 REPORTED_ATTRIBUTE,
                                                 (uint8_t *)&my_home_id,
                                                 sizeof(zwave_home_id_t),
                                                 0);
}

attribute_store_node_t get_zpc_node_id_node()
{
  zwave_node_id_t my_node_id = zwave_network_management_get_node_id();

  return attribute_store_get_node_child_by_value(get_zpc_network_node(),
                                                 ATTRIBUTE_NODE_ID,
                                                 REPORTED_ATTRIBUTE,
                                                 (uint8_t *)&my_node_id,
                                                 sizeof(zwave_node_id_t),
                                                 0);
}

attribute_store_node_t get_zpc_endpoint_id_node(zwave_endpoint_id_t endpoint_id)
{
  return attribute_store_get_node_child_by_value(get_zpc_node_id_node(),
                                                 ATTRIBUTE_ENDPOINT_ID,
                                                 REPORTED_ATTRIBUTE,
                                                 (uint8_t *)&endpoint_id,
                                                 sizeof(zwave_endpoint_id_t),
                                                 0);
}

///////////////////////////////////////////////////////////////////////////////
// Initialization specifically for ZPC attribute_store
///////////////////////////////////////////////////////////////////////////////
static sl_status_t invoke_update_callbacks_in_network()
{
  attribute_store_node_t home_id = get_zpc_network_node();

  // First off, find all network statuses and set them from "included" to "Unavailable"
  zpc_attribute_store_update_all_network_statuses(
    home_id,
    ZCL_NODE_STATE_NETWORK_STATUS_ONLINE_FUNCTIONAL,
    ZCL_NODE_STATE_NETWORK_STATUS_UNAVAILABLE);

  sl_status_t refresh
    = attribute_store_refresh_node_and_children_callbacks(home_id);

  if (refresh != SL_STATUS_OK) {
    sl_log_error(LOG_TAG,
                 "Failed to trigger a refresh callback for attribute Store "
                 "node %d. This node should represent our HomeID");
    return refresh;
  }

  // Set all nodes as Included, ready to be operated
  zpc_attribute_store_update_all_network_statuses(
    home_id,
    ZCL_NODE_STATE_NETWORK_STATUS_UNAVAILABLE,
    ZCL_NODE_STATE_NETWORK_STATUS_ONLINE_FUNCTIONAL);

  sl_log_info(LOG_TAG,
              "Refreshed callbacks on Attribute Node %d, "
              "representing our current HomeID",
              home_id);
  return refresh;
}

sl_status_t zpc_attribute_store_init()
{
  sl_status_t status = zpc_attribute_store_register_known_attribute_types();

  // For all other attribute, fill default registration data
  status |= zpc_attribute_store_register_default_attribute_type_data();

  // Configure the attribute store.
  // Save in the worst case scenario every 10 minutes if we keep changing the
  // Attribute Store aggressively
  attribute_store_configuration_set_auto_save_safety_interval(10 * 60);
  // Wait 10 seconds since the last Attribute Store change before saving
  // to the datastore.
  attribute_store_configuration_set_auto_save_cooldown_interval(10);
  attribute_store_configuration_set_type_validation(true);

  // Configure the Unify DotDot Attribute Store component:
  unify_dotdot_attribute_store_set_configuration(&zpc_configuration);

  // Just simulate an update of the whole Attribute Store.
  status |= invoke_update_callbacks_in_network();
  return status;
}
