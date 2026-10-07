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

#ifndef COMMAND_CLASS_METER_H
#define COMMAND_CLASS_METER_H

#include "command_class_meter_mqtt.hpp"
#include "command_class_meter_attribute_store.hpp"

#include <utility>
#include <vector>

namespace zwave_command_class
{

    class command_class_meter final : public command_class_meter_attribute_store, public command_class_meter_mqtt
    {

        public:
            command_class_meter();
            ~command_class_meter() = default;

            void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version) override;

            sl_status_t on_meter_supported_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_meter_attribute_map_t payload) override;
            sl_status_t on_meter_report_parsed(const zwave_controller_connection_info_t *connection_info, attribute_store::attribute endpoint, command_class_meter_attribute_map_t payload) override;

            sl_status_t on_meter_get_requested_assemble_frame(const get_requested_args &args, uint8_t *data, uint16_t *length) override;
            sl_status_t on_meter_reset_requested_assemble_frame(const set_requested_args &args, uint8_t *data, uint16_t *length) override;

        private:
            static std::vector<uint8_t> scales_from_supported_bitmask(uint8_t version, uint8_t scale_supported_0, uint8_t m_s_t, const std::vector<uint8_t> &scale_supported_follow);
            static std::vector<uint8_t> rate_types_for_meter_get(uint8_t version, uint8_t supported_rate_type_masked);

            static std::vector<std::pair<uint8_t, uint8_t>> reading_requests_for_endpoint(attribute_store::attribute endpoint_node, uint8_t version);
            static bool start_meter_get(attribute_store::attribute endpoint_node, uint8_t scale, uint8_t rate_type);
            static bool start_next_missing_meter_get(attribute_store::attribute endpoint_node, uint8_t version);
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_METER_H
