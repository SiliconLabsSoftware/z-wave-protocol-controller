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

#ifndef OTA_MQTT_API_HPP
#define OTA_MQTT_API_HPP

#include "mqtt_api_base.hpp"
#include "update_manager_types.hpp"
#include "safe_queue.hpp"

#include <string>

namespace ota
{

    /**
     * @brief MQTT API for the OTA Firmware Manager.
     *
     * Subscribes to 7 command topics and publishes to 7 report topics.
     * Incoming commands are translated into ota_external_event_data and pushed onto the
     * shared event queue for processing by the state machine thread.
     */
    class OTAMqttApi : public zwave_command_class::MqttApiBase
    {
        public:
            inline static std::string MQTT_API_OTA_UPLOAD_IMAGE_TOPIC                 = "OTA/UploadImage";
            inline static std::string MQTT_API_OTA_UPLOAD_IMAGE_REPORT_TOPIC          = MQTT_API_OTA_UPLOAD_IMAGE_TOPIC + "/Report";
            inline static std::string MQTT_API_OTA_START_FIRMWARE_UPLOAD_TOPIC        = "OTA/StartFirmwareUpload";
            inline static std::string MQTT_API_OTA_START_FIRMWARE_UPLOAD_REPORT_TOPIC = MQTT_API_OTA_START_FIRMWARE_UPLOAD_TOPIC + "/Report";
            inline static std::string MQTT_API_OTA_LIST_IMAGES_TOPIC                  = "OTA/ListImages";
            inline static std::string MQTT_API_OTA_LIST_IMAGES_REPORT_TOPIC           = MQTT_API_OTA_LIST_IMAGES_TOPIC + "/Report";
            inline static std::string MQTT_API_OTA_REMOVE_IMAGE_TOPIC                 = "OTA/RemoveImage";
            inline static std::string MQTT_API_OTA_REMOVE_IMAGE_REPORT_TOPIC          = MQTT_API_OTA_REMOVE_IMAGE_TOPIC + "/Report";
            inline static std::string MQTT_API_OTA_PROGRESS_TOPIC                     = "OTA/Progress";
            inline static std::string MQTT_API_OTA_PROGRESS_REPORT_TOPIC              = MQTT_API_OTA_PROGRESS_TOPIC + "/Report";
            inline static std::string MQTT_API_OTA_ABORT_TOPIC                        = "OTA/Abort";
            inline static std::string MQTT_API_OTA_ABORT_REPORT_TOPIC                 = MQTT_API_OTA_ABORT_TOPIC + "/Report";
            inline static std::string MQTT_API_OTA_ACTIVATE_TOPIC                     = "OTA/Activate";
            inline static std::string MQTT_API_OTA_ACTIVATE_REPORT_TOPIC              = MQTT_API_OTA_ACTIVATE_TOPIC + "/Report";

            /**
             * @brief Construct the MQTT API.
             * @param event_queue  Reference to the shared event queue.
             */
            OTAMqttApi(::threading::safe_queue<ota_external_event_data> &event_queue);
            ~OTAMqttApi() = default;

            void setup_mqtt_api() override;

            using zwave_command_class::MqttApiBase::publish_report;

        private:
            // ---- Command handlers ----
            static void on_upload_image(const std::string &topic, const std::string &message);
            void on_start_firmware_upload(const std::string &topic, const std::string &message);
            static void on_list_images(const std::string &topic, const std::string &message);
            static void on_remove_image(const std::string &topic, const std::string &message);
            void on_progress(const std::string &topic, const std::string &message);
            void on_abort(const std::string &topic, const std::string &message);
            void on_activate(const std::string &topic, const std::string &message);

            ::threading::safe_queue<ota_external_event_data> &event_queue;
    };

}  // namespace ota

#endif  // OTA_MQTT_API_HPP
