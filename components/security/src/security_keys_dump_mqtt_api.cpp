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

#include "security_keys_dump_mqtt_api.hpp"
#include "seal_x25519.hpp"
#include "security_keys_dump_writer.hpp"

#include "fmt/format.h"
#include "log.h"
#include "nlohmann/json.hpp"
#include "zpc_config.h"
#include "zwave_network_management.h"
#include "zwave_s2_keystore.h"

#include <openssl/crypto.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <vector>

namespace zwave_command_class
{
    namespace
    {
        constexpr std::string_view LOG_TAG = "security_keys_dump_mqtt_api";

        // 6 YAML lines, each "<class_name>: '<hex_encoded_key>'\n". Longest
        // class name is "S2_AUTHENTICATED_LR" (19), longest line = 56 B,
        // total max ~300 B.
        constexpr std::size_t KEY_BLOB_CAPACITY = 512;

        // Render a SHA-256 fingerprint as lowercase hex.
        std::string sha256_fingerprint_to_hex(const std::array<uint8_t, 32> &h)
        {
            std::string out;
            out.reserve(64);
            for (uint8_t b: h) {
                out += fmt::format("{:02x}", b);
            }
            return out;
        }

        /**
         * mlock() prevents the kernel from paging this buffer to swap, so
         * cleartext keys cannot leak to the swap partition while sealing.
         */
        class MlockGuard
        {
            public:
                MlockGuard(void *p, std::size_t n) : p_(p), n_(n)
                {
                    if (mlock(p_, n_) != 0) {
                        sl_log_warning(LOG_TAG.data(),
                                       "mlock(%zu B) for plaintext keys buffer failed: %s. "
                                       "Proceeding without swap protection.",
                                       n_,
                                       std::strerror(errno));
                    }
                }
                ~MlockGuard()
                {
                    OPENSSL_cleanse(p_, n_);
                    (void)munlock(p_, n_);  // No-op if mlock failed; nothing useful to do on error.
                }
                MlockGuard(const MlockGuard &)            = delete;
                MlockGuard &operator=(const MlockGuard &) = delete;

            private:
                void *p_;
                std::size_t n_;
        };
    }  // namespace

    void SecurityKeysDumpMqttApi::setup_mqtt_api()
    {
        subscribe_topic(SecurityKeysDumpMqttApi::MQTT_API_DUMP_SECURITY_KEYS_TOPIC, [](const std::string &topic, const std::string &message) { zwave_command_class::SecurityKeysDumpMqttApi::on_dump_security_keys(topic, message); });
    }

    sl_status_t SecurityKeysDumpMqttApi::initialize()
    {
        setup_mqtt_api();
        return SL_STATUS_OK;
    }

    int SecurityKeysDumpMqttApi::shutdown()
    {
        return 0;
    }

    std::string SecurityKeysDumpMqttApi::name() const
    {
        return "Security Keys Dump MQTT API";
    }

    void SecurityKeysDumpMqttApi::publish_report_status(report_status status)
    {
        nlohmann::json report;
        report["status"] = static_cast<int>(status);
        publish_report(MQTT_API_DUMP_SECURITY_KEYS_REPORT_TOPIC, report.dump(), false);
    }

    void SecurityKeysDumpMqttApi::on_dump_security_keys(const std::string &topic, const std::string &message)
    {
        (void)topic;
        (void)message;

        // 1. Feature must be explicitly enabled in config.
        const zpc_config_t *cfg = zpc_get_config();
        if (cfg == nullptr || !cfg->security_keys_dump_enable) {
            sl_log_warning(LOG_TAG.data(), "Dump request received but feature is disabled");
            publish_report_status(report_status::feature_disabled);
            return;
        }

        const std::string pubkey_path = (cfg->security_keys_dump_recipient_pubkey_path != nullptr) ? cfg->security_keys_dump_recipient_pubkey_path : "";
        const std::string out_dir     = (cfg->security_keys_dump_output_dir != nullptr) ? cfg->security_keys_dump_output_dir : "";
        if (pubkey_path.empty() || out_dir.empty()) {
            sl_log_error(LOG_TAG.data(), "Config missing pubkey_path='%s' output_dir='%s'", pubkey_path.c_str(), out_dir.c_str());
            publish_report_status(report_status::missing_config);
            return;
        }

        const std::string out_path = fmt::format("{}/{:08X}.bin", out_dir, zwave_network_management_get_home_id());

        // 2. Collect the plaintext keys blob into a local buffer
        std::vector<uint8_t> plaintext(KEY_BLOB_CAPACITY, 0);
        MlockGuard plaintext_guard(plaintext.data(), plaintext.size());
        std::size_t plaintext_len = 0;
        sl_status_t st            = zwave_s2_collect_security_keys_blob(plaintext.data(), plaintext.size(), &plaintext_len);
        if (st != SL_STATUS_OK) {
            sl_log_error(LOG_TAG.data(), "Failed to collect keys blob: %d", static_cast<int>(st));
            publish_report_status(report_status::collect_failed);
            return;
        }
        if (plaintext_len == 0) {
            sl_log_info(LOG_TAG.data(), "No S2 keys are currently assigned");
        }

        // 3. Seal the plaintext to the recipient public key.
        std::vector<uint8_t> envelope;
        std::array<uint8_t, 32> recipient_fp {};
        st = security::seal_to_x25519_pem(pubkey_path, plaintext.data(), plaintext_len, envelope, &recipient_fp);

        if (st != SL_STATUS_OK) {
            const report_status seal_status = (st == SL_STATUS_NOT_FOUND || st == SL_STATUS_INVALID_TYPE || st == SL_STATUS_FAIL) ? report_status::pubkey_load_failed : report_status::seal_failed;
            sl_log_error(LOG_TAG.data(), "seal_to_x25519_pem failed (%d) for pubkey '%s'", static_cast<int>(st), pubkey_path.c_str());
            publish_report_status(seal_status);
            return;
        }

        // 4. Write the ciphertext atomically with hardened perms.
        st = security::write_security_keys_dump(out_path, envelope.data(), envelope.size());
        if (st != SL_STATUS_OK) {
            sl_log_error(LOG_TAG.data(), "write_security_keys_dump('%s') failed (%d)", out_path.c_str(), static_cast<int>(st));
            publish_report_status(report_status::file_write_failed);
            return;
        }

        // 5. Audit log. Records the recipient pubkey fingerprint.
        sl_log_info(LOG_TAG.data(),
                    "Dump OK: wrote %zu encrypted bytes to '%s' "
                    "(recipient_fp=%s, plaintext_len=%zu)",
                    envelope.size(),
                    out_path.c_str(),
                    sha256_fingerprint_to_hex(recipient_fp).c_str(),
                    plaintext_len);

        publish_report_status(report_status::ok);
    }
}  // namespace zwave_command_class
