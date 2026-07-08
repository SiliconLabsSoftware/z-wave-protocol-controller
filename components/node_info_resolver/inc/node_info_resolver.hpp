
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

#ifndef NODE_INFO_RESOLVER_H
#define NODE_INFO_RESOLVER_H

#include <any>
#include "zwave_keyset_definitions.h"
#include "zwave_generic_types.h"
#include "zwave_controller_connection_info.h"
#include "zwave_controller_callbacks.h"
#include "init_builder.hpp"

namespace zwave_command_class
{
    class node_info_resolver : public Initializable
    {
        public:
            node_info_resolver();
            ~node_info_resolver() = default;

            // Initializable interface
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

        private:
            static void on_node_information_update(zwave_node_id_t node_id, const zwave_node_info_t *node_info);
            static void on_nif_resolution_failed(attribute_store_node_t node);
            static void on_secure_node_info_no_response(attribute_store_node_t node);
            static sl_status_t resolve_node_info(attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length);
            static sl_status_t resolve_secure_node_info(attribute_store_node_t node, uint8_t *frame, uint16_t *frame_length);
            static void on_non_secure_nif_update(attribute_store_node_t node, attribute_store_change_t change);
            static void on_granted_security_key_update(attribute_store_node_t node, attribute_store_change_t change);
            static void create_secure_nifs_if_missing(attribute_store_node_t node_id_node);
    };
}  // namespace zwave_command_class
#endif  // NODE_INFO_RESOLVER_H
