/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include "steps/ota_step_waiting_for_reconnect.hpp"

#include "zwave_tx.h"
#include "zwave_tx_scheme_selector.h"
#include "log.h"

#include <chrono>
#include <future>
#include <string_view>
#include <thread>

namespace ota
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_step_waiting_for_reconnect";

    static constexpr uint8_t NOP_FRAME[]        = {0x00};
    static constexpr uint16_t NOP_RETRY_DELAY_S = 5;
    static constexpr uint16_t MAX_NOP_RETRIES   = 60;

    static std::promise<bool> *s_nop_promise = nullptr;

    static void on_nop_tx_complete(uint8_t tx_status, const zwapi_tx_report_t *tx_report, void *user)
    {
        if (s_nop_promise != nullptr) {
            s_nop_promise->set_value(tx_status == TRANSMIT_COMPLETE_OK);
            s_nop_promise = nullptr;
        }
    }

    std::string OtaStepWaitingForReconnect::name() const
    {
        return "OTA Step Waiting For Reconnect";
    }

    bool OtaStepWaitingForReconnect::handles_external_event(ota_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult OtaStepWaitingForReconnect::on_enter(OtaSession &session)
    {
        sl_log_info(LOG_TAG.data(), "Node %d: waiting %u s (WaitTime) before NOP probe", session.node_id, session.wait_time);

        std::this_thread::sleep_for(std::chrono::seconds(session.wait_time));

        for (uint16_t attempt = 0; attempt < MAX_NOP_RETRIES; ++attempt) {
            if (send_nop_sync(session)) {
                sl_log_info(LOG_TAG.data(), "Node %d: NOP ACK received, proceeding to re-interview", session.node_id);
                return done();
            }
            sl_log_warning(LOG_TAG.data(), "Node %d: NOP failed (attempt %u/%u), retrying in %u s", session.node_id, attempt + 1, MAX_NOP_RETRIES, NOP_RETRY_DELAY_S);
            std::this_thread::sleep_for(std::chrono::seconds(NOP_RETRY_DELAY_S));
        }

        sl_log_error(LOG_TAG.data(), "Node %d: NOP probe failed after %u retries, giving up", session.node_id, MAX_NOP_RETRIES);
        return fail();
    }

    StepResult OtaStepWaitingForReconnect::handle_event(OtaSession &session, std::optional<ota_external_event_data> event)
    {
        (void)session;
        (void)event;
        return stay();
    }

    bool OtaStepWaitingForReconnect::send_nop_sync(OtaSession &session)
    {
        std::promise<bool> promise;
        auto future   = promise.get_future();
        s_nop_promise = &promise;

        zwave_controller_connection_info_t connection_info = {};
        zwave_tx_scheme_get_node_connection_info(session.node_id, 0, &connection_info);

        zwave_tx_options_t tx_options                  = {};
        constexpr uint8_t number_of_expected_responses = 0;
        constexpr uint32_t discard_timeout_ms          = 10000;
        zwave_tx_scheme_get_node_tx_options(ZWAVE_TX_QOS_RECOMMENDED_GET_ANSWER_PRIORITY, number_of_expected_responses, discard_timeout_ms, &tx_options);

        sl_status_t status = zwave_tx_send_data(&connection_info, sizeof(NOP_FRAME), NOP_FRAME, &tx_options, on_nop_tx_complete, nullptr, nullptr);

        if (status != SL_STATUS_OK) {
            s_nop_promise = nullptr;
            sl_log_error(LOG_TAG.data(), "Failed to send NOP to node %d (status=0x%04X)", session.node_id, status);
            return false;
        }

        return future.get();
    }

}  // namespace ota
