
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

#include <cstddef>
#include <fmt/base.h>
#include <fmt/format.h>
#include <string_view>

// Base class
#include "command_class_powerlevel.hpp"

// Z-Wave defintions
#include "ZW_classcmd.h"
#include "zwave_tx.h"
#include "zwave_command_class_utils.hpp"

#include "log.h"

namespace zwave_command_class
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "command_class_powerlevel";

    command_class_powerlevel::powerlevel_setting_t command_class_powerlevel::power_level_setting = {};
    command_class_powerlevel::power_level_test_t command_class_powerlevel::power_level_test      = {};

    command_class_powerlevel::command_class_powerlevel()
    {
        // Constructor body - can be empty or contain initialization logic
    }

    void command_class_powerlevel::reset_power_level(void *data)
    {
        timer_stop(&power_level_setting.timer);
        power_level_setting.power_level     = NORMAL_POWER;
        power_level_setting.expiration_time = 0;
        zwapi_set_rf_power_level(power_level_setting.power_level);
    }

    void command_class_powerlevel::set_power_level(rf_power_level_t new_power_level, uint8_t timeout)
    {
        power_level_setting.power_level = new_power_level;

        if (new_power_level == NORMAL_POWER) {
            reset_power_level(nullptr);
            return;
        }
        zwapi_set_rf_power_level(power_level_setting.power_level);
        timer_set(&power_level_setting.timer, timeout * CLOCK_SECOND, command_class_powerlevel::reset_power_level, 0);
        power_level_setting.expiration_time = clock_time() + (timeout * CLOCK_SECOND);
    }

    void command_class_powerlevel::power_level_test_send_data_callback(uint8_t status, const zwapi_tx_report_t *tx_status, void *user)
    {
        // Decrease the remaining amount of frames to send
        power_level_test.frame_count--;

        if (status == TRANSMIT_COMPLETE_OK) {
            power_level_test.acknowledged_frames_count++;
        }

        if (power_level_test.frame_count > 0) {
            if (zwave_tx_send_test_frame(power_level_test.destination_node, power_level_test.power_level, power_level_test_send_data_callback, NULL, NULL) != SL_STATUS_OK) {
                // TX queue refused our frame, it's kind of bad. We don't want to stress
                // it more by going into a crazy recursion, especially for a power level
                // test. Let's just abort the test by makind a callback for the final frame.
                power_level_test.frame_count = 1;
                power_level_test_send_data_callback(TRANSMIT_COMPLETE_FAIL, NULL, NULL);
            }
            return;
        }

        if (power_level_test.acknowledged_frames_count == 0) {
            power_level_test.status = TEST_FAILED;
        } else {
            power_level_test.status = TEST_SUCCESSFUL;
        }

        // Send a notification to the controlling node when we are done
        std::vector<uint8_t> frame(6);
        frame[0] = COMMAND_CLASS_POWERLEVEL;
        frame[1] = static_cast<uint8_t>(command_class_powerlevel_commands_t::COMMAND_CLASS_POWERLEVEL_POWERLEVEL_TEST_NODE_REPORT);
        frame[2] = static_cast<uint8_t>(power_level_test.destination_node);
        frame[3] = static_cast<uint8_t>(power_level_test.power_level);
        frame[4] = static_cast<uint8_t>((power_level_test.acknowledged_frames_count >> 8) & 0xFF);
        frame[5] = static_cast<uint8_t>((power_level_test.acknowledged_frames_count >> 0) & 0xFF);
        command_class_utils::send_report(&power_level_test.connection, static_cast<uint16_t>(frame.size()), frame.data());
    }

    sl_status_t command_class_powerlevel::on_powerlevel_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_powerlevel_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        if ((connection_info != nullptr) && connection_info->local.is_multicast) {
            return SL_STATUS_OK;
        }

        uint8_t timeout = 0;
        if (power_level_setting.expiration_time > 0) {
            timeout = (uint8_t)((power_level_setting.expiration_time - clock_time()) / 1000);
        }

        report_frame.add_raw_byte((uint8_t)power_level_setting.power_level);
        report_frame.add_raw_byte(timeout);

        frame = report_frame.generate_frame();

        return SL_STATUS_OK;
    }

    sl_status_t command_class_powerlevel::on_powerlevel_test_node_get_support_requested_assemble_frame(const zwave_controller_connection_info_t *connection_info, command_class_powerlevel_attribute_map_t attribute_map, zwave_frame_generator_standalone &report_frame, std::vector<uint8_t> &frame)
    {
        if ((connection_info != nullptr) && connection_info->local.is_multicast) {
            return SL_STATUS_OK;
        }

        report_frame.add_raw_byte((uint8_t)power_level_test.destination_node);
        report_frame.add_raw_byte((uint8_t)power_level_test.status);
        report_frame.add_raw_byte((power_level_test.acknowledged_frames_count >> 8) & 0xFF);
        report_frame.add_raw_byte((power_level_test.acknowledged_frames_count >> 0) & 0xFF);

        frame = report_frame.generate_frame();

        return SL_STATUS_OK;
    }

    sl_status_t command_class_powerlevel::on_powerlevel_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_powerlevel_attribute_map_t attribute_map)
    {
        uint8_t power_level = 0;
        power_level         = get_value_or_default(attribute_map, "power_level", power_level);
        uint8_t timeout     = 0;
        timeout             = get_value_or_default(attribute_map, "timeout", timeout);

        if (power_level > MINIMUM_POWER) {
            sl_log_debug(LOG_TAG.data(),
                         "Powerlevel Set command powerlevel value is unknown"
                         "(value = %d). Ignoring.",
                         power_level);
            return SL_STATUS_FAIL;
        }

        if (power_level == NORMAL_POWER) {
            reset_power_level(nullptr);
            sl_log_debug(LOG_TAG.data(), "Power level set to normal");
            return SL_STATUS_OK;
        }
        if (timeout == 0) {
            sl_log_warning(LOG_TAG.data(), "Power level won't be set because timeout is 0");
            return SL_STATUS_FAIL;
        }
        set_power_level((rf_power_level_t)power_level, timeout);
        sl_log_debug(LOG_TAG.data(), "Power level set to %d with timeout %d", power_level, timeout);

        return SL_STATUS_OK;
    }

    sl_status_t command_class_powerlevel::on_powerlevel_test_node_set_support_received(const zwave_controller_connection_info_t *connection_info, command_class_powerlevel_attribute_map_t attribute_map)
    {
        uint8_t power_level       = 0;
        power_level               = get_value_or_default(attribute_map, "power_level", power_level);
        uint16_t test_frame_count = 0;
        test_frame_count          = get_value_or_default(attribute_map, "test_frame_count", test_frame_count);
        uint8_t test_nodeid       = 0;
        test_nodeid               = get_value_or_default(attribute_map, "test_nodeid", test_nodeid);

        if (power_level_test.frame_count > 0) {
            // Another test ongoing, we support only 1 concurrent test
            return SL_STATUS_FAIL;
        }

        if (power_level > MINIMUM_POWER) {
            return SL_STATUS_FAIL;
        }

        power_level_test.frame_count = test_frame_count;
        if (power_level_test.frame_count == 0) {
            return SL_STATUS_FAIL;
        }

        power_level_test.status                    = TEST_IN_PROGRESS;
        power_level_test.destination_node          = test_nodeid;
        power_level_test.power_level               = (rf_power_level_t)power_level;
        power_level_test.acknowledged_frames_count = 0;
        power_level_test.connection                = *connection_info;

        if (zwave_tx_send_test_frame(power_level_test.destination_node, power_level_test.power_level, power_level_test_send_data_callback, NULL, NULL) != SL_STATUS_OK) {
            // TX queue refused our frame, we consider it as a Transmit Failed, but
            // the send data callback will try once more before aborting the test
            // completely.
            power_level_test_send_data_callback(TRANSMIT_COMPLETE_FAIL, NULL, NULL);
        }

        return SL_STATUS_OK;
    }

}  // namespace zwave_command_class