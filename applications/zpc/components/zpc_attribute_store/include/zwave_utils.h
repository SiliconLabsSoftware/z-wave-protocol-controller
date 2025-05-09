/**
 * @file zwave_utils.h
 * @brief Various Z-Wave related utilities.
 *
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 */

#ifndef ZWAVE_UTILS_H
#define ZWAVE_UTILS_H

// Generic includes
#include <stdint.h>
#include <stdbool.h>

// Interfaces
#include "sl_status.h"
#include "zwave_generic_types.h"
#include "zwave_node_id_definitions.h"
#include "zwave_controller_keyset.h"
#include "zwave_keyset_definitions.h"
#include "zwave_command_class_version_types.h"
#include "zwave_command_class_wake_up_types.h"
#include "attribute_store_defined_attribute_types.h"

#include "attribute_store.h"

/**
 * @ingroup zpc_utils
 * @{
 */

/**
 * @addtogroup zwave_utils Z-Wave Utilities
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns whether a node supports Network Layer Security (NLS)
 *
 * It will search for the node in the Attribute store and deduct its NLS support
 * from the ATTRIBUTE_ZWAVE_NLS_SUPPORT
 * attribute stored under the NodeID.
 *
 * @param node_id The NodeID for which the NLS support is requested
 * @param value_state The value state to look for
 * 
 * @returns
 * - true if the node/endpoint supports NLS
 * - false if the node/endpoint does not support NLS
 */
bool zwave_get_nls_support(zwave_node_id_t node_id, attribute_store_node_value_state_t value_state);

/**
 * @brief Returns whether a node has Network Layer Security (NLS) enabled
 *
 * It will search for the node in the Attribute store and deduct its NLS state
 * from the ATTRIBUTE_ZWAVE_NLS_STATE
 * attribute stored under the NodeID.
 *
 * @param node_id The NodeID for which the NLS state is requested
 * @param value_state The value state to look for
 * 
 * @returns
 * - true if the node/endpoint has NLS enabled
 * - false if the node/endpoint has NLS disabled
 */
bool zwave_get_nls_state(zwave_node_id_t node_id, attribute_store_node_value_state_t value_state);

/**
 * @brief Stores Network Layer Security (NLS) support for a NodeID in the attribute store.
 *
 * @param node_id                 The NodeID for which the NLS support
 *                                must be saved in the attribute store
 * @param is_nls_supported        NLS support
 * @param value_state             The value state to store
 * 
 * @returns SL_STATUS_OK on success, any other code if not successful.
 */
sl_status_t zwave_store_nls_support(zwave_node_id_t node_id,
                                    bool is_nls_supported,
                                    attribute_store_node_value_state_t value_state);

/**
 * @brief Stores Network Layer Security (NLS) state for a NodeID in the attribute store.
 *
 * @param node_id                 The NodeID for which the NLS state
 *                                must be saved in the attribute store
 * @param is_nls_enabled          NLS state
 * @param value_state             The value state to store
 * 
 * @returns SL_STATUS_OK on success, any other code if not successful.
 */
sl_status_t zwave_store_nls_state(zwave_node_id_t node_id,
                                  bool is_nls_enabled,
                                  attribute_store_node_value_state_t value_state);

/**
 * @brief Returns the operating mode of a node
 *
 * Refer to the Z-Wave Network Layer Specification for Operating Modes.
 * It can be Always listening (AL), Frequently listening (FL) or
 * Non-Listening (NL).
 *
 * It will search for the node in the Attribute store and deduct its operating
 * mode from the ATTRIBUTE_ZWAVE_PROTOCOL_LISTENING
 * and ATTRIBUTE_ZWAVE_OPTIONAL_PROTOCOL
 * attributes stored under the NodeID.
 *
 * @param node_id The NodeID for which the operating mode is requested
 * @returns zwave_operating_mode_t value.
 */
zwave_operating_mode_t zwave_get_operating_mode(zwave_node_id_t node_id);

/**
 * @brief Returns the protocol that the node is running in this network
 *
 * This function will return the protocol that has been used for including
 * a node in our network. After inclusion, nodes run only 1 protocol.
 *
 * It will search for the node in the Attribute store and deduct its protocol
 * from the ATTRIBUTE_ZWAVE_INCLUSION_PROTOCOL
 * attribute stored under the NodeID.
 *
 * @param node_id The NodeID for which the operating protocol is requested
 * @returns zwave_protocol_t value.
 */
zwave_protocol_t zwave_get_inclusion_protocol(zwave_node_id_t node_id);

/**
 * @brief Returns the name of a protocol value
 *
 * @param protocol zwave_protocol_t value to print out
 *
 * @returns A string pointer to the name of the Z-Wave Protocol.
 */
const char *zwave_get_protocol_name(zwave_protocol_t protocol);

/**
 * @brief Stores a protocol value for a NodeID in the attribute store.
 *
 * @param node_id                 The NodeID for which the operating protocol
 *                                must be saved in the attribute store
 * @param inclusion_protocol      zwave_protocol_t value to save
 * @returns SL_STATUS_OK on success, any other code if not successful.
 */
sl_status_t zwave_store_inclusion_protocol(zwave_node_id_t node_id,
                                           zwave_protocol_t inclusion_protocol);

/**
 * @brief Get the node granted keys object
 *
 * @param node_id
 * @param keys
 */
sl_status_t zwave_get_node_granted_keys(zwave_node_id_t node_id,
                                        zwave_keyset_t *keys);

/**
 * @brief Set the node granted keys object
 *
 * @param node_id
 * @param keys
 */
sl_status_t zwave_set_node_granted_keys(zwave_node_id_t node_id,
                                        zwave_keyset_t *keyset);

/**
 * @brief Verify whether a node/endpoint supports a Command Class
 * using the attribute Store.
 *
 * @param command_class The Command Class identifier to look for.
 * @param node_id       The NodeID to verify for support.
 * @param endpoint_id   The Endpoint to verify for support.

 * @returns
 * - true if the node/endpoint supports the Command Class
 * - false if the node/endpoint does not support the Command Class
 */
bool zwave_node_supports_command_class(zwave_command_class_t command_class,
                                       zwave_node_id_t node_id,
                                       zwave_endpoint_id_t endpoint_id);

/**
 * @brief Verify whether a node/endpoint supports Supervision CC 
 * AND wants to use it.
 *
 * @param node_id       The NodeID to verify for support.
 * @param endpoint_id   The Endpoint to verify for support.

 * @returns
 * - true if the node/endpoint supports the Supervision CC + has the enabled supervision flag (or if the flag is not defined)
 * - false otherwise
 */
bool zwave_node_want_supervision_frame(zwave_node_id_t node_id,
                                       zwave_endpoint_id_t endpoint_id);

/**
 * @brief Return the version of a Command Class implemented by a node.
 *
 * @param command_class The Command Class identifier for which the version
 *                      is requested.
 * @param node_id       The NodeID for which the version is requested.
 * @param endpoint_id   The Endpoint for which the version is requested.

 * @returns the version number of the Command Class. 0 if it is
 *          neither supported nor controlled.
 */
zwave_cc_version_t
  zwave_node_get_command_class_version(zwave_command_class_t command_class,
                                       zwave_node_id_t node_id,
                                       zwave_endpoint_id_t endpoint_id);

/**
 * @brief Get endpoint node associated with the ZWave NodeID and Endpoint ID
 * 
 * @param node_id       The Z-Wave NodeID for which we want to find the endpoint
 * @param endpoint_id   The Z-Wave Endpoint ID for which we want to find the endpoint
 * 
 * @returns The attribute store node associated with the endpoint.
 */
attribute_store_node_t zwave_get_endpoint_node(zwave_node_id_t node_id,
                                               zwave_endpoint_id_t endpoint_id);
  /**
 * @brief Verify if we registered a node as supporting S2.
 *
 * @param node_id             The NodeID that supports S2.
 * @returns True if the node has been registered to support S2, false otherwise
 */
  bool zwave_security_validation_is_node_s2_capable(zwave_node_id_t node_id);

/**
 * @brief Sets a node as S2 capable, meaning that we know it supports S2,
 *        and it will forever do.
 *
 * @param node_id             The NodeID that supports S2.
 */
sl_status_t
  zwave_security_validation_set_node_as_s2_capable(zwave_node_id_t node_id);

/**
 * @brief Navigates the attribute store to find out what is the Wake Up Interval
 *  of a node.
 *
 * It will search under all endpoints, (despite that the setting should be under
 * endpoint 0) and return the first found value.
 *
 * @param node_id       The Z-Wave NodeID for which we want to find out the
 *                      Wake Up Interval.
 * @returns The value of the Wake Up interval. 0 is returned if there is no
 *          Wake up interval of if it is unknown.
 */
wake_up_interval_t zwave_get_wake_up_interval(zwave_node_id_t node_id);

/**
 * @brief Searches if a Command Class/Command pair is in a byte array of
 * Command class/command pairs
 *
 * @param command_class         The Command Class to look for
 * @param command               The Command to look for
 * @param command_list          Pointer to the Command Class/Command byte array
 * @param command_list_length   Length of the command_list array (in bytes)
 *
 * @returns true if the Command Class/Command pair has been found, false if not found
 */
bool is_command_in_array(zwave_command_class_t command_class,
                         zwave_command_t command,
                         const uint8_t *command_list,
                         uint8_t command_list_length);

/**
 * @brief Verifies if an endpoint exists under a NodeID
 *
 * @param node_id         NodeID for which we want to verify endpoint existence
 * @param endpoint_id     Endpoint ID to look for
 * @returns true if the endpoint exists, false otherwise
 */
bool endpoint_exists(zwave_node_id_t node_id, zwave_endpoint_id_t endpoint_id);

/** @} (end addtogroup zwave_utils) */
/** @} (end addtogroup zpc_utils) */
#ifdef __cplusplus
}
#endif

#endif /* ZWAVE_UTILS_H */
