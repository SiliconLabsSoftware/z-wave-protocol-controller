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

#include "steps/ota_step_upload_prepare.hpp"
#include "ota_image_store.hpp"
#include "attribute_resolver.h"
#include "log.h"

#include <string_view>

namespace ota
{

    [[maybe_unused]] static constexpr std::string_view LOG_TAG = "ota_step_upload_prepare";

    std::string OtaStepUploadPrepare::name() const
    {
        return "OTA Step Upload Prepare";
    }

    bool OtaStepUploadPrepare::handles_external_event(ota_external_event_t event_type) const
    {
        (void)event_type;
        return false;
    }

    void OtaStepUploadPrepare::pause_resolution(OtaSession &session)
    {
        attribute_resolver_pause_node_resolution(session.endpoint_node);
        session.resolution_paused = true;
        sl_log_info(LOG_TAG.data(), "Paused attribute resolution for node %d", session.node_id);
    }

    StepResult OtaStepUploadPrepare::on_enter(OtaSession &session)
    {
        sl_log_info(LOG_TAG.data(), "Preparing firmware transfer for node %d", session.node_id);
        session.transfer.bytes_transferred = 0;
        session.transfer.abort_requested   = false;
        session.firmware_image_cache       = ota::OTAImageStore::get_image(session.upload.image_name);

        if (session.firmware_image_cache.empty()) {
            sl_log_error(LOG_TAG.data(), "Failed to load image '%s' from store", session.upload.image_name.c_str());
            return fail();
        }

        session.transfer.image_size = static_cast<uint32_t>(session.firmware_image_cache.size());

        sl_log_info(LOG_TAG.data(),
                    "Loaded image '%s' (%zu bytes), "
                    "max_fragment_size=%u",
                    session.upload.image_name.c_str(),
                    session.firmware_image_cache.size(),
                    session.firmware_md.max_fragment_size);

        pause_resolution(session);

        session.upload_in_progress = true;
        return done();
    }

    StepResult OtaStepUploadPrepare::handle_event(OtaSession &session, std::optional<ota_external_event_data> event)
    {
        (void)session;
        (void)event;
        return stay();
    }

}  // namespace ota
