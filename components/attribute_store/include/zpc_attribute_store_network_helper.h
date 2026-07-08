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

#ifndef ATTRIBUTE_STORE_NETWORK_HELPER_H
#define ATTRIBUTE_STORE_NETWORK_HELPER_H

// Includes from this component
#include "attribute_store.h"

// Includes from other components
#include "sl_status.h"
#include "zwave_controller_types.h"  // for zwave_endpoint_id_t

/**
 * @defgroup zpc_attribute_store_network_helpers ZPC Attribute Store Z-Wave Network Helpers
 * @ingroup zpc_attribute_store
 * @brief Helper functions to read find out HomeID / NodeID / Endpoints
 * nodes in the @ref attribute_store
 *
 * These helper functions return the attribute store nodes for a given Home ID and
 * Node ID.
 * Note that if requested node does not exist in the @ref attribute_store, they will
 * be created and their newly created node will be returned.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return the attribute store node representing the HomeID.
 *
 * Note: If the HomeID does not exist in the attribute store,
 * this function will create it.
 *
 * @param home_id Z-Wave Home ID
 *
 * @returns attribute_store_node_t representing the HomeID in the attribute store
 */
attribute_store_node_t attribute_store_network_helper_create_home_id_node(zwave_home_id_t home_id);

/**
 * @brief Return the attribute store node representing the NodeID under a Home ID.
 *
 * Note: If the NodeID does not exist in the attribute store,
 * this function will create it.
 *
 * @param home_id Z-Wave Home ID
 * @param node_id Z-Wave Node ID
 *
 * @returns attribute_store_node_t representing the NodeID in the attribute store
 */
attribute_store_node_t attribute_store_network_helper_create_node_id_node(zwave_home_id_t home_id, zwave_node_id_t node_id);

/**
 * @brief Return the attribute store node for a Home ID / Node ID / endpoint.
 *
 * Note: If the endpoint does not exist in the attribute store, this function
 * will create it.

 * @param home_id     Z-Wave Home ID
 * @param node_id     Z-Wave Node ID
 * @param endpoint_id Endpoint ID under the Node ID
 *
 * @returns attribute_store_node_t representing the endpoint in the attribute store
 */
attribute_store_node_t attribute_store_network_helper_create_endpoint_node(zwave_home_id_t home_id, zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id);

/**
 * @brief Ensures a Z-Wave NodeID node exists in the attribute store with endpoint 0.
 *
 * Used by network_monitor and from synchronous Z-Wave callbacks that must observe a
 * consistent node placeholder before connector events (e.g. external node ID assignment).
 *
 * @param node_id  Z-Wave node identifier
 *
 * @returns NodeID attribute store node, or ATTRIBUTE_STORE_INVALID_NODE if the node could
 *          not be created
 */
attribute_store_node_t attribute_store_network_helper_ensure_zwave_node_placeholder(zwave_node_id_t node_id);

/**
 * @brief Return the attribute store node representing the HomeID.
 *
 * @param home_id Z-Wave Home ID
 *
 * @returns attribute_store_node_t representing the HomeID in the attribute store.
 * @returns ATTRIBUTE_STORE_INVALID_NODE if no matching Home ID node exists
 */
attribute_store_node_t attribute_store_network_helper_get_home_id_node(zwave_home_id_t home_id);

/**
 * @brief Return the attribute store node representing the NodeID under a Home ID.
 *
 * @param home_id Z-Wave Home ID
 * @param node_id Z-Wave Node ID
 *
 * @returns attribute_store_node_t representing the NodeID in the attribute store.
 * @returns ATTRIBUTE_STORE_INVALID_NODE if no matching node exists
 */
attribute_store_node_t attribute_store_network_helper_get_node_id_node(zwave_home_id_t home_id, zwave_node_id_t node_id);

/**
 * @brief Take a Z-Wave NodeID and return its Attribute Store node representing the NodeID.
 *
 * This function resolves the Node ID using the current Z-Wave network management
 * Home ID, ensuring the returned node belongs to the ZPC's network.
 *
 * @param node_id The Z-Wave NodeID value to parse and search in the tree.
 *
 * @returns attribute_store_node_t representing the NodeID for the Z-Wave
 *          NodeID in the attribute store.
 * @returns ATTRIBUTE_STORE_INVALID_NODE if no matching Z-Wave NodeID node exists
 */
attribute_store_node_t attribute_store_network_helper_get_zwave_node_id_node(zwave_node_id_t node_id);

/**
 * @brief Return the attribute store node for a Home ID / Node ID / endpoint.
 *
 * @param home_id     Z-Wave Home ID
 * @param node_id     Z-Wave Node ID
 * @param endpoint_id Endpoint ID under the Node ID
 *
 * @returns attribute_store_node_t representing the endpoint in the attribute store
 * @returns ATTRIBUTE_STORE_INVALID_NODE if no matching node exists
 */
attribute_store_node_t attribute_store_network_helper_get_endpoint_node(zwave_home_id_t home_id, zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id);

/**
 * @brief Returns the Attribute Store node for Endpoint 0, under a NodeID node.
 *
 * @param node_id_node   The attribute store node for the NodeID.

 * @returns attribute_store_node_t representing Endpoint ID 0 under the NodeID
 */
attribute_store_node_t attribute_store_get_endpoint_0_node(attribute_store_node_t node_id_node);

/**
 * @brief Traverse the tree up from a node and read Home ID, Node ID, and endpoint.
 *
 * @param node        Attribute store node to start from
 * @param home_id     Output: Home ID
 * @param node_id     Output: Node ID
 * @param endpoint_id Output: Endpoint ID
 *
 * @returns SL_STATUS_OK        If Home ID / Node ID / endpoint parents were found
 * @returns SL_STATUS_NOT_FOUND If the tree was traversed without finding the data
 * @returns SL_STATUS_FAIL      If reading values from the attribute store failed
 */
sl_status_t attribute_store_network_helper_get_home_node_endpoint_from_node(attribute_store_node_t node, zwave_home_id_t *home_id, zwave_node_id_t *node_id, zwave_endpoint_id_t *endpoint_id);

/**
 * @brief Traverse up the tree from a node and read Home ID and Node ID.
 *
 * @param node    Attribute store node to start from
 * @param home_id Output: Home ID
 * @param node_id Output: Node ID
 *
 * @returns SL_STATUS_OK        If Home ID / Node ID parents were found
 * @returns SL_STATUS_NOT_FOUND If the tree was traversed without finding the data
 * @returns SL_STATUS_FAIL      If reading values from the attribute store failed
 */
sl_status_t attribute_store_network_helper_get_home_and_node_id_from_node(attribute_store_node_t node, zwave_home_id_t *home_id, zwave_node_id_t *node_id);

/**
 * @brief Traverse up the tree from a node and finds under which Z-Wave NodeID it is.
 *
 * @param node          The Attribute store node for which the parent NodeID
 *                      will be search for
 * @param zwave_node_id A pointer where to write the found Z-Wave NodeID
 *
 * @returns SL_STATUS_OK        If a NodeID type of parents node was
 *                              found and its reported value was copied in the
 *                              zwave_node_id variable
 * @returns SL_STATUS_NOT_FOUND If the function went all the way up to the tree root
 *                              and did not find NodeID type of node
 * @returns SL_STATUS_FAIL      If some error occurred reading values in the tree.
 */
sl_status_t attribute_store_network_helper_get_node_id_from_node(attribute_store_node_t node, zwave_node_id_t *zwave_node_id);

/**
 * @brief Traverse up the tree from a node and finds under which Z-Wave endpoint id it is located.
 *
 * @param node              The Attribute store node for which the parent endpoint
 *                          will be searched for
 * @param zwave_endpoint_id A pointer where to write the found Z-Wave endpoint id
 *
 * @returns SL_STATUS_OK        If an endpoint type of parents node was
 *                              found and its value was copied in the
 *                              zwave_endpoint_id variable
 * @returns SL_STATUS_NOT_FOUND If the function went all the way up to the tree root
 *                              and did not find any endpoint type of attribute
 * @returns SL_STATUS_FAIL      If some error occurred reading values in the tree.
 */
sl_status_t attribute_store_network_helper_get_endpoint_id_from_node(attribute_store_node_t node, zwave_endpoint_id_t *zwave_endpoint_id);

/**
 * @brief Traverse up the tree from a node and finds under which Z-Wave NodeID/endpoint id it is located.
 *
 * @param node              The Attribute store node for which the parent Z-Wave NodeID/endpoint
 *                          will be searched for
 * @param zwave_node_id     A pointer where to write the found Z-Wave NodeID (reported value)
 * @param zwave_endpoint_id A pointer where to write the found Z-Wave endpoint id (reported value)
 *
 * @returns SL_STATUS_OK        If both NodeID/endpoint parents nodes were
 *                              found and their value were copied in the
 *                              zwave_node_id and zwave_endpoint_id variables
 * @returns SL_STATUS_NOT_FOUND If the function went all the way up to the tree root
 *                              and did not find consecutive NodeID/endpoint attribute types
 * @returns SL_STATUS_FAIL      If some error occurred reading values in the tree.
 */
sl_status_t attribute_store_network_helper_get_zwave_ids_from_node(attribute_store_node_t node, zwave_node_id_t *zwave_node_id, zwave_endpoint_id_t *zwave_endpoint_id);

/**
 * @brief Gets the Attribute Store Endpoint ID node for a given
 *        Z-Wave NodeID / Endpoint ID in our network
 *
 * @param node_id           Z-Wave NodeID to find in the attribute store
 * @param endpoint_id       Z-Wave Endpoint ID to find in the attribute store.
 * @returns Attribute store node for the NodeID / Endpoint.
 */
attribute_store_node_t zwave_command_class_get_endpoint_id_node(zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id);

#ifdef __cplusplus
}
#endif

/** @} end attribute_store_network_helpers */

#endif  // ATTRIBUTE_STORE_NETWORK_HELPER_H
