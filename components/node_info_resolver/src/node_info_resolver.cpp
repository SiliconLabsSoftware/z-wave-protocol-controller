
/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>
#include <any>

// Interfaces
#include "attribute_store_defined_attribute_types.h"
#include "zwave_helper_macros.h"

// ZPC components
#include "zwave_network_management.h"
#include "zwave_utils.h"
#include "zpc_attribute_resolver.h"
#include "zpc_attribute_store.h"
#include "zpc_attribute_store_network_helper.h"
#include "zwave_tx.h"
#include "zwave_tx_scheme_selector.h"
#include "zwave_controller_utils.h"
#include "zwave_security_validation.h"

// Includes from ZPC components
#include "attribute_store.h"
#include "attribute_timeouts.h"
#include "attribute_store_helper.h"
#include "attribute_resolver.h"
#include "attribute_resolver.hpp"   // attribute_resolver::register_rules
#include "attribute_callbacks.hpp"  // attribute_store::register_callback_by_type_and_state

#include "log.h"

#include "node_info_resolver.hpp"
#include "node_info_resolver_events.hpp"
#include "node_info_resolver_types.hpp"

#include "component_connector.hpp"

#include "command_class_s2_events.hpp"
#include "command_class_s2_types.hpp"
#include "command_class_s0_events.hpp"
#include "command_class_s0_types.hpp"
#include "command_class_multi_channel_events.hpp"
#include "command_class_multi_channel_types.hpp"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "node_info_resolver";

    node_info_resolver::node_info_resolver()
    {
        // Constructor is minimal - initialization happens in initialize()
    }

    void node_info_resolver::on_node_information_update(zwave_node_id_t node_id, const zwave_node_info_t *node_info)
    {
        zwave_home_id_t home_id                 = zwave_network_management_get_home_id();
        attribute_store_node_t endpoint_id_node = attribute_store_network_helper_get_endpoint_node(home_id, node_id, 0);

        attribute_store_node_t non_secure_nif_node = attribute_store_get_first_child_by_type(endpoint_id_node, ATTRIBUTE_ZWAVE_NIF);

        // Pack the NIF data into a nif array
        // The size is 2 * ZWAVE_CONTROLLER_MAXIMUM_COMMAND_CLASS_LIST_LENGTH because Generic Extended Command Classes are 2-byte long
        uint8_t nif[ZWAVE_CONTROLLER_MAXIMUM_COMMAND_CLASS_LIST_LENGTH * 2];
        uint8_t nif_length = 0;
        zwave_command_class_list_pack(node_info, nif, &nif_length);

        // Security time
        // Before we trust that input, verify if some Security CC just disappeared
        // from the NIF, when it should not have.
        if (zwave_security_validation_is_s2_nif_downgrade_attack_detected(node_id, nif, nif_length)) {
            if (nif_length < sizeof(nif)) {
                // Keep S2 in the NIF !
                nif[nif_length] = COMMAND_CLASS_SECURITY_2;
                nif_length += 1;
            } else {
                // Overflow, just return and toss the faulty NIF.
                return;
            }
        }

        attribute_store_set_reported(non_secure_nif_node, nif, nif_length);

        // We just wrote down a new NIF.
        // Verify if it is the first time that we detect a node supports S2/S0:
        if (zwave_node_supports_command_class(COMMAND_CLASS_SECURITY_2, node_id, 0)) {
            zwave_security_validation_set_node_as_s2_capable(node_id);
        }

        // Save the additional data: (Protocol listening / Optional protocol)
        attribute_store_node_t node_id_node = attribute_store_get_first_parent_with_type(endpoint_id_node, ATTRIBUTE_NODE_ID);
        attribute_store_set_child_reported(node_id_node, ATTRIBUTE_ZWAVE_PROTOCOL_LISTENING, &node_info->listening_protocol, sizeof(node_info->listening_protocol));

        // Find the optional protocol byte from the NIF
        attribute_store_set_child_reported(node_id_node, ATTRIBUTE_ZWAVE_OPTIONAL_PROTOCOL, &node_info->optional_protocol, sizeof(node_info->optional_protocol));

        // Save all the device type information.
        attribute_store_set_child_reported(endpoint_id_node, ATTRIBUTE_ZWAVE_BASIC_DEVICE_CLASS, &node_info->basic_device_class, sizeof(node_info->basic_device_class));

        attribute_store_set_child_reported(endpoint_id_node, ATTRIBUTE_ZWAVE_GENERIC_DEVICE_CLASS, &node_info->generic_device_class, sizeof(node_info->generic_device_class));

        attribute_store_set_child_reported(endpoint_id_node, ATTRIBUTE_ZWAVE_SPECIFIC_DEVICE_CLASS, &node_info->specific_device_class, sizeof(node_info->specific_device_class));
    }

    void node_info_resolver::on_nif_resolution_failed(attribute_store_node_t nif_node)
    {
        sl_log_debug(LOG_TAG.data(), "NIF Get Resolution failed for Attribute ID %d", nif_node);
        attribute_store_node_t endpoint_node = attribute_store_get_first_parent_with_type(nif_node, ATTRIBUTE_ENDPOINT_ID);
        zwave_endpoint_id_t endpoint_id      = 0;
        attribute_store_get_reported(endpoint_node, &endpoint_id, sizeof(endpoint_id));

        if (endpoint_id != 0) {
            sl_log_debug(LOG_TAG.data(),
                         "Deleting Endpoint %d (Attribute ID %d) "
                         "as we cannot resolve its NIF",
                         endpoint_id,
                         endpoint_node);
            attribute_store_delete_node(endpoint_node);
        }
    }

    void node_info_resolver::on_secure_node_info_no_response(attribute_store_node_t secure_nif_node)
    {
        uint8_t nif[ZWAVE_CONTROLLER_MAXIMUM_COMMAND_CLASS_LIST_LENGTH];
        uint8_t nif_length = 0;

        if ((SL_STATUS_OK == attribute_store_get_node_attribute_value(secure_nif_node, REPORTED_ATTRIBUTE, nif, &nif_length)) && (nif_length != 0)) {
            return;
        }

        sl_log_warning(LOG_TAG.data(), "Failed to get S2 capabilities Attribute ID %d", secure_nif_node);

        // Are we trying to resolve our own NIF?
        attribute_store_node_t node_id_node = attribute_store_get_first_parent_with_type(secure_nif_node, ATTRIBUTE_NODE_ID);

        if (get_zpc_node_id_node() == node_id_node) {
            return;
        }

        // Find the Endpoint ID for who we want the NIF.
        zwave_endpoint_id_t endpoint_id         = 0;
        attribute_store_node_t endpoint_id_node = attribute_store_get_first_parent_with_type(secure_nif_node, ATTRIBUTE_ENDPOINT_ID);
        if (endpoint_id_node == ATTRIBUTE_STORE_INVALID_NODE) {
            return;
        }
        if (SL_STATUS_OK != attribute_store_get_reported(endpoint_id_node, &endpoint_id, sizeof(zwave_endpoint_id_t))) {
            // Abort if no Endpoint ID data was retrieved.
            return;
        }

        // If not a Multi Channel endpoint, nothing to do
        if (endpoint_id <= 0) {
            return;
        }
        attribute_store_node_t non_secure_nif_node = attribute_store_get_first_child_by_type(endpoint_id_node, ATTRIBUTE_ZWAVE_NIF);

        if ((non_secure_nif_node != ATTRIBUTE_STORE_INVALID_NODE) && (SL_STATUS_OK == attribute_store_get_node_attribute_value(non_secure_nif_node, REPORTED_ATTRIBUTE, nif, &nif_length))) {
            uint8_t j = 0;
            for (uint8_t i = 0; i < nif_length; i++) {
                if (nif[i] != COMMAND_CLASS_SECURITY && nif[i] != COMMAND_CLASS_SECURITY_2) {
                    nif[j++] = nif[i];
                }
            }
            nif_length = j;

            // The implicit rule that all non-secure command classes for an End Point
            // must be controllable securely is still in effect,
            // if the endpoint is reported secure.
            attribute_store_set_reported(secure_nif_node, nif, nif_length);
        }
    }

    sl_status_t node_info_resolver::resolve_node_info(attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
    {
        // Are we trying to resolve our own NIF?
        attribute_store_node_t node_id_node = attribute_store_get_first_parent_with_type(node, ATTRIBUTE_NODE_ID);

        if (get_zpc_node_id_node() == node_id_node) {
            attribute_store_delete_node(node);
            *frame_length = 0;
            return SL_STATUS_ALREADY_EXISTS;
        }

        // Find the Endpoint ID for who we want the NIF.
        zwave_endpoint_id_t endpoint_id = 0;
        if (SL_STATUS_OK != attribute_store_network_helper_get_endpoint_id_from_node(node, &endpoint_id)) {
            *frame_length = 0;
            return SL_STATUS_FAIL;
        }

        if (endpoint_id == 0) {
            // Root device NIF, we used Request NIF from the Protocol CC.
            node_info_resolver_types::request_node_info_frame_t *request_nif = (node_info_resolver_types::request_node_info_frame_t *)frame;
            request_nif->command_class                                       = 0x01;  // TODO: Use ZWAVE_PROTOCOL_COMMAND_CLASS when zwave_command_class_indices.h is migrated
            request_nif->command                                             = 0x02;  // TODO: Use ZWAVE_NODE_INFO_REQUEST_COMMAND when zwave_command_class_indices.h is migrated
            *frame_length                                                    = sizeof(node_info_resolver_types::request_node_info_frame_t);
            return SL_STATUS_OK;
        }

        return SL_STATUS_OK;
    }

    sl_status_t node_info_resolver::resolve_secure_node_info(attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length)
    {
        // Here it could be either S0 or S2.
        // Base the decision on granted keys (better than CC support in this case)

        // Find the Z-Wave NodeID (endpoint does not matter here, it's the same command).
        zwave_node_id_t node_id = 0;
        if (SL_STATUS_OK != attribute_store_network_helper_get_node_id_from_node(node, &node_id)) {
            return SL_STATUS_FAIL;
        }

        zwave_keyset_t supporting_node_keys = 0;
        zwave_get_node_granted_keys(node_id, &supporting_node_keys);

        zwave_controller_encapsulation_scheme_t supporting_node_scheme = zwave_controller_get_highest_encapsulation(supporting_node_keys);

        if (zwave_controller_encapsulation_scheme_greater_equal(supporting_node_scheme, ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_2_UNAUTHENTICATED)) {
            return SL_STATUS_OK;
        }

        if (supporting_node_scheme == ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_0) {
            return SL_STATUS_OK;
        }

        *frame_length = 0;
        return SL_STATUS_FAIL;
    }

    void node_info_resolver::on_non_secure_nif_update(attribute_store_node_t updated_node, attribute_store_change_t change)
    {
        if (change == ATTRIBUTE_DELETED) {
            return;
        }

        // Verify if it is time to create a S2 capabilities for all endpoints
        if (attribute_store_is_value_defined(updated_node, REPORTED_ATTRIBUTE)) {
            attribute_store_node_t node_id_node = attribute_store_get_first_parent_with_type(updated_node, ATTRIBUTE_NODE_ID);

            create_secure_nifs_if_missing(node_id_node);
        }
    }

    void node_info_resolver::on_granted_security_key_update(attribute_store_node_t updated_node, attribute_store_change_t change)
    {
        if (change == ATTRIBUTE_DELETED) {
            return;
        }

        // Verify if it is time to create a S2 capabilities for all endpoints
        if (attribute_store_is_value_defined(updated_node, REPORTED_ATTRIBUTE)) {
            attribute_store_node_t node_id_node = attribute_store_get_first_parent_with_type(updated_node, ATTRIBUTE_NODE_ID);

            create_secure_nifs_if_missing(node_id_node);
        }
    }

    void node_info_resolver::create_secure_nifs_if_missing(attribute_store_node_t node_id_node)
    {
        zwave_node_id_t node_id = 0;
        attribute_store_get_reported(node_id_node, &node_id, sizeof(node_id));

        if (node_id == 0 || node_id_node == get_zpc_node_id_node()) {
            // Don't do that for ourselves
            return;
        }
        // Verify if it is time to create a S2 capabilities, for all endpoints under a NodeID:

        // If S0 or S2 keys have been granted, then we create this secure NIF
        // We don't really care if the S0/S2 CCs are present in the non-secure NIF.
        zwave_keyset_t supporting_node_keys = 0;
        zwave_get_node_granted_keys(node_id, &supporting_node_keys);
        zwave_controller_encapsulation_scheme_t supporting_node_scheme = zwave_controller_get_highest_encapsulation(supporting_node_keys);

        if (zwave_controller_encapsulation_scheme_greater_equal(supporting_node_scheme, ZWAVE_CONTROLLER_ENCAPSULATION_SECURITY_0)) {
            // Showtime! They have a security key!
            // Find the NodeID in the attribute store, and verify all endpoints.
            uint32_t current_child                  = 0;
            attribute_store_node_t endpoint_id_node = attribute_store_get_node_child_by_type(node_id_node, ATTRIBUTE_ENDPOINT_ID, current_child);
            current_child += 1;

            const attribute_store_type_t attribute[] = {ATTRIBUTE_ZWAVE_SECURE_NIF};
            while (endpoint_id_node != ATTRIBUTE_STORE_INVALID_NODE) {
                attribute_store_add_if_missing(endpoint_id_node, attribute, COUNT_OF(attribute));

                endpoint_id_node = attribute_store_get_node_child_by_type(node_id_node, ATTRIBUTE_ENDPOINT_ID, current_child);
                current_child += 1;
            }
        }
    }

    sl_status_t node_info_resolver::initialize()
    {
        sl_log_info(LOG_TAG.data(), "Node Info Resolver initialized");

        static zwave_controller_callbacks_t node_info_resolver_callbacks = {
          .on_node_information = node_info_resolver::on_node_information_update,
        };

        zwave_controller_register_callbacks(&node_info_resolver_callbacks);

        return SL_STATUS_OK;
    }

    int node_info_resolver::shutdown()
    {
        return 0;
    }

    std::string node_info_resolver::name() const
    {
        return "Node Info Resolver";
    }

}  // namespace zwave_command_class