
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

#ifndef GRANTED_KEYS_RESOLVER_H
#define GRANTED_KEYS_RESOLVER_H

#include <any>
#include "attribute_store.h"
#include "zwave_keyset_definitions.h"
#include "zwave_generic_types.h"
#include "zwave_controller_connection_info.h"
#include "init_builder.hpp"

namespace zwave_command_class
{
    class granted_keys_resolver : public Initializable
    {
        public:
            struct zwave_key_protocol_combination_t {
                    zwave_keyset_t key;
                    zwave_protocol_t protocol;
            };

            granted_keys_resolver();
            ~granted_keys_resolver() = default;

            // Initializable interface
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

            /**
             * @brief Updates the Attribute Store to indicate that the Security key and
             * Protocol that we just used are working with this node.
             *
             * @param node_id_node    Attribute Store node for the NodeID.
             * @param encapsulation   Encapsulation that was used successfully with the node
             *
             */
            static void mark_key_protocol_as_supported(attribute_store_node_t node_id_node, zwave_controller_encapsulation_scheme_t encapsulation);

        private:
            static void on_zwave_key_probe_send_data_complete(uint8_t status, const zwapi_tx_report_t *tx_info, void *user);

            static sl_status_t probe_key_and_protocol(attribute_store_node_t key_protocol_node, uint8_t *frame, uint16_t *frame_length);
            static sl_status_t get_inclusion_protocol(attribute_store_node_t protocol_node, uint8_t *frame, uint16_t *frame_length);
            static sl_status_t get_granted_keys(attribute_store_node_t keyset_node, uint8_t *frame, uint16_t *frame_length);

            /**
             * @brief Reacts to Node Info Frame updates and verify that we will search
             * for granted keys/inclusion protocol next.
             *
             * @param node        Attribute Store Node that is updated
             * @param change      Attribute Store changed applied to the node.
             */
            static void on_node_info_update(attribute_store_node_t node, attribute_store_change_t change);

            /**
             * @brief Checks if the granted keys were just set to 0. In this case,
             * we automatically adjust the Inclusion Protocol to be Z-Wave
             *
             * @param granted_keys_node   Attribute Store Node for the granted keys
             * @param change              Attribute Store change applied to the node.
             */
            static void on_granted_keys_update(attribute_store_node_t granted_keys_node, attribute_store_change_t change);

            /**
             * @brief Checks if the inclusion protocol just got updated to UNKNOWN, and
             * undefine it if that's the case (because we know how to resolve it now)
             *
             * @param protocol_node       Attribute Store Node for the inclusion protocol
             * @param change              Attribute Store change applied to the node.
             */
            static void on_inclusion_protocol_update(attribute_store_node_t protocol_node, attribute_store_change_t change);

            /**
             * @brief Create a list of keys/protocol objects to resolve
             *
             * @param node_id_node      Attribute Store Node for the NodeID.
             * @param s2_support        flag indicating if S2 is in the NIF
             * @param s0_support        flag indicating if S0 is in the NIF
             */
            static void create_list_of_keys_protocol_to_resolve(attribute_store_node_t node_id_node, bool s2_support, bool s0_support);

            /**
             * @brief Verifies if the Non-secure NIF is already resolved.
             *
             * @param node_id_node      Attribute Store for the NodeID.
             * @returns if the NodeID's NIF is known and resolved, false otherwise.
             */
            static bool is_nif_resolved(attribute_store_node_t node_id_node);

            /**
             * @brief Remove some discovery combinations if we know the protocol
             *
             * Combinations with a different protocols will be removed.
             *
             * @param node_id_node      Attribute Store node for the NodeId
             * @param protocol          Z-Wave Protocol that we want too keep.
             */
            static void exclude_combinations_with_protocol_from_discovery(attribute_store_node_t node_id_node, zwave_protocol_t protocol);
    };
}  // namespace zwave_command_class
#endif  // GRANTED_KEYS_RESOLVER_H
