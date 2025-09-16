/******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#include "multilevel_sensor_cluster_server.h"
#include "zcl_cluster_servers_helpers.hpp"

// Interfaces
#include "zwave_command_class_version_types.h"
#include "zwave_command_class_configuration_types.h"

// ZPC includes
#include "zpc_attribute_store.h"
#include "zpc_attribute_store_network_helper.h"
#include "zwave_command_class_generic_types.h"
#include "attribute_store_defined_attribute_types.h"

// Includes from Unify shared components
#include "attribute.hpp"
#include "attribute_store_helper.h"
#include "sl_log.h"

// Includes from auto-generated files
#include "dotdot_mqtt.h"
#include "zap-types.h"
#include "dotdot_mqtt_helpers.hpp"

// Generic includes
#include <string>
#include <stdlib.h>
#include <vector>

using namespace attribute_store;

// Setup Log ID
constexpr char LOG_TAG[] = "multilevel_sensor_cluster_server";

// Attribute macro, shortening those long defines for attribute types:
#define ATTRIBUTE(type) ATTRIBUTE_COMMAND_CLASS_SENSOR_MULTILEVEL_##type

// List of attributes of SensorValues
namespace
{
const std::vector<attribute_store_type_t> sensor_values_attributes
  = {ATTRIBUTE(SENSOR_VALUE), ATTRIBUTE(SCALE)};
}

///////////////////////////////////////////////////////////////////////////////
// Attribute publication functions
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief Publishes the Multilevel Sensor Cluster Server attributes 
 *
 * @param unid        		 unid for which we want to publish the
 *                            	SensorValues attributes.
 * @param endpoint_id        Endpoint ID for which we want to publish the
 *                            SensorValues attributes.
 * @param sensor_type        Sensor Type node ID for which we want to publish the
 *                            SensorValues attributes.
 */
static sl_status_t publish_multilevel_sensor_cluster_attributes(
  const std::string &unid,
  attribute_store::attribute sensor_type_node,
  zwave_endpoint_id_t endpoint_id)
{
  // Do not publish any state supported commands for ourselves.
  if (is_zpc_unid(unid.c_str())) {
    return SL_STATUS_FAIL;
  }

  // Build the base topic and pass it on to DotDot MQTT.
  try {
    // Get reported sensor type ID
    uint8_t sensor_type = sensor_type_node.reported<uint8_t>();

    // Get SensorType name
    const std::string sensor_type_str
      = multilevel_sensor_sensor_type_get_enum_value_name(sensor_type);

    // Added sensor type name to base topic
    const std::string base_topic = "ucl/by-unid/" + std::string(unid) + "/ep"
                                   + std::to_string(endpoint_id) + "/"
                                   + std::string(sensor_type_str);

    SensorValue value = {0, 0};
    // Get report sensor value
    attribute_store::attribute sensor_value_node
      = sensor_type_node.child_by_type(ATTRIBUTE(SENSOR_VALUE));
    if (sensor_value_node.reported_exists()) {
      value.Value = sensor_value_node.reported<int32_t>();
    }
    // Get report sensor scale
    attribute_store::attribute sensor_scale_node
      = sensor_type_node.child_by_type(ATTRIBUTE(SCALE));
    if (sensor_scale_node.reported_exists()) {
      value.Scale = static_cast<uint8_t>(sensor_scale_node.reported<int32_t>());
    }

    // Pulish the sensor value attribute
    if (SL_STATUS_OK
        != uic_mqtt_dotdot_multilevel_sensor_sensor_values_publish(
          base_topic.c_str(),
          value,
          UCL_MQTT_PUBLISH_TYPE_REPORTED)) {
      return SL_STATUS_FAIL;
    }
  } catch (const std::exception &e) {
    sl_log_error(LOG_TAG,
                 "Error while get base topic and sensor data  : %s",
                 e.what());

    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Attribute store callback functions
//////////////////////////////////////////////////////////////////////////////
/**
 * @brief Listens to updates to the SensorValues then publishes the attributes.
 *
 * @param updated_node    Attribute Store node that was modified.
 * @param change          Type of change applied to the node.
 */
void on_sensor_values_update(attribute_store_node_t updated_node,
                             attribute_store_change_t change)
{
  if (change == ATTRIBUTE_CREATED || change == ATTRIBUTE_DELETED) {
    return;
  }

  // Go up and find the UNID/Endpoint and its network status.
  unid_t unid;
  zwave_endpoint_id_t endpoint_id = 0;
  if (SL_STATUS_OK
      != attribute_store_network_helper_get_unid_endpoint_from_node(
        updated_node,
        unid,
        &endpoint_id)) {
    return;
  }

  attribute_store::attribute sensor_type_node
    = attribute_store_get_first_parent_with_type(updated_node,
                                                 ATTRIBUTE(SENSOR_TYPE));

  // Publish the multilevel sensor values:
  if (SL_STATUS_OK
      != publish_multilevel_sensor_cluster_attributes(std::string(unid),
                                                      sensor_type_node,
                                                      endpoint_id)) {
    return;
  }
}

///////////////////////////////////////////////////////////////////////////////
//  Init and teardown functions.
//////////////////////////////////////////////////////////////////////////////
sl_status_t multilevel_sensor_cluster_server_init(void)
{
  sl_log_debug(LOG_TAG, "Multilevel sensor server initialization");

  // Register attribute updates

  attribute_store_register_callback_by_type_to_array(
    &on_sensor_values_update,
    sensor_values_attributes.data(),
    static_cast<uint32_t>(sensor_values_attributes.size()));

  return SL_STATUS_OK;
}
