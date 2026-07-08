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

/**
 * On request, ZPC collects its currently-assigned S2 (and S0, if any) keys
 * into memory, encrypts them to a pre-configured X25519 recipient public
 * key, and writes the resulting envelope to a configured path. The MQTT
 * Report payload contains only a status code; no key material is published.
 *
 * The feature is OFF by default. To enable it, set in the config file:
 *
 *     security.security_keys_dump_enable: true
 *     security.security_keys_dump_recipient_pubkey_path: "/etc/zpc/dump_pub.pem"
 *     security.security_keys_dump_output_path: "/var/lib/zpc/keys_dump.bin"
 *
 * See `doc/security_keys_dump_mqtt_api.md` for details.
 */

#ifndef SECURITY_KEYS_DUMP_MQTT_API_HPP
#define SECURITY_KEYS_DUMP_MQTT_API_HPP

#include "mqtt_api_base.hpp"
#include "sl_status.h"

#include <string>
#include <string_view>

namespace zwave_command_class
{
    /**
     * @brief MQTT API for the encrypted security keys dump feature.
     */
    class SecurityKeysDumpMqttApi : public MqttApiBase, public Initializable
    {
        public:
            SecurityKeysDumpMqttApi()           = default;
            ~SecurityKeysDumpMqttApi() override = default;

            void setup_mqtt_api() override;
            sl_status_t initialize() override;
            int shutdown() override;
            std::string name() const override;

        private:
            inline static const std::string MQTT_API_DUMP_SECURITY_KEYS_TOPIC        = "Network/DumpSecurityKeys";
            inline static const std::string MQTT_API_DUMP_SECURITY_KEYS_REPORT_TOPIC = MQTT_API_DUMP_SECURITY_KEYS_TOPIC + "/Report";

            enum class report_status : int {
                ok                 = 0,
                feature_disabled   = 1,
                missing_config     = 2,
                pubkey_load_failed = 3,
                collect_failed     = 4,
                seal_failed        = 5,
                file_write_failed  = 6,
            };

            static void on_dump_security_keys(const std::string &topic, const std::string &message);
            static void publish_report_status(report_status status);
    };
}  // namespace zwave_command_class

#endif  // SECURITY_KEYS_DUMP_MQTT_API_HPP
