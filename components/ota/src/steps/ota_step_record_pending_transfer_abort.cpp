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

#include "steps/ota_step_record_pending_transfer_abort.hpp"

namespace ota
{

    std::string OtaStepRecordPendingTransferAbort::name() const
    {
        return "OTA Step Record Pending Transfer Abort";
    }

    bool OtaStepRecordPendingTransferAbort::handles_external_event(ota_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    StepResult OtaStepRecordPendingTransferAbort::on_enter(OtaSession &session)
    {
        session.transfer.abort_requested = true;
        return done();
    }

    StepResult OtaStepRecordPendingTransferAbort::handle_event(OtaSession &session, std::optional<ota_external_event_data> event)
    {
        (void)session;
        (void)event;
        return stay();
    }

}  // namespace ota
