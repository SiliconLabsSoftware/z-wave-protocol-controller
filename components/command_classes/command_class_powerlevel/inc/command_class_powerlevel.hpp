
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

#ifndef COMMAND_CLASS_POWERLEVEL_H
#define COMMAND_CLASS_POWERLEVEL_H

#include "command_class_powerlevel_attribute_store.hpp"

#include "zwapi_protocol_basis.h"
#include "zwapi_protocol_transport.h"
#include "timer.hpp"

namespace zwave_command_class
{

    class command_class_powerlevel final : public command_class_powerlevel_attribute_store
    {

        public:
            command_class_powerlevel();
            ~command_class_powerlevel() = default;

        private:
            /**
             * Possible values for Powerlevel test status
             */
            typedef enum {
                ///< None of the frames were acknowdledged by the destination
                TEST_FAILED = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED,
                ///< Test completed and at least one frame was acknowledged by the destination
                TEST_SUCCESSFUL = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_SUCCES,
                ///< Test is ongoing
                TEST_IN_PROGRESS = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS,
            } power_level_test_status_t;

            /**
             * Powerlevel configuration
             */
            typedef struct power_level_setting {
                    /// Current power level setting.
                    rf_power_level_t power_level;
                    /// timer used to rollback the power to normal.
                    struct timer_handle_t timer;
                    /// Timestamp of when the powerlevel will is set back to normal.
                    /// 0 if we are already at the normal level.
                    uint64_t expiration_time;
            } powerlevel_setting_t;

            /**
             * Settings and state of powerlevel test.
             */
            typedef struct power_level_test {
                    ///< NodeID that asked us to perform a powerlevel test
                    zwave_controller_connection_info_t connection;
                    ///< Destination NodeID for the power level test
                    zwave_node_id_t destination_node;
                    ///< Powerlevel to use for the test
                    rf_power_level_t power_level;
                    ///< Number of frames to left transmit for the test
                    uint16_t frame_count;
                    ///< Number of frames that have been acknowledged
                    uint16_t acknowledged_frames_count;
                    ///< Status of the power level test
                    power_level_test_status_t status;
            } power_level_test_t;

            static powerlevel_setting_t power_level_setting;
            static power_level_test_t power_level_test;

            static void reset_power_level(void *data);
            static void set_power_level(rf_power_level_t new_power_level, uint8_t timeout);
            static void power_level_test_send_data_callback(uint8_t status, const zwapi_tx_report_t *tx_status, void *user);

            sl_status_t on_powerlevel_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_powerlevel_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;
            sl_status_t on_powerlevel_test_node_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_powerlevel_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame) override;

            sl_status_t on_powerlevel_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_powerlevel_attribute_map_t attribute_map) override;
            sl_status_t on_powerlevel_test_node_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_powerlevel_attribute_map_t attribute_map) override;
    };

}  // namespace zwave_command_class

#endif  // COMMAND_CLASS_POWERLEVEL_H
