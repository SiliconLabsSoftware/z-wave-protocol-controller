#include "aes_cmac.hpp"

#include "psa/crypto.h"

namespace crypto
{

    sl_status_t Crypto::aes_cmac_compute(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t> message, std::span<uint8_t, CRYPTO_AES_CMAC_OUTPUT_LEN> mac) const
    {
        psa_key_id_t key_id;
        psa_status_t status;
        size_t mac_len;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_wrappers_.psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.import_key(PSA_KEY_USAGE_SIGN_MESSAGE, PSA_ALG_CMAC, key.data(), &key_id, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS, CRYPTO_AES_KEY_SIZE);
        if (status != PSA_SUCCESS) {
            return psa_wrappers_.psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.mac_compute(key_id, PSA_ALG_CMAC, message.data(), message.size(), mac.data(), CRYPTO_AES_CMAC_OUTPUT_LEN, &mac_len);
        psa_wrappers_.destroy_key(key_id);
        if (status == PSA_SUCCESS && mac_len != CRYPTO_AES_CMAC_OUTPUT_LEN) {
            return SL_STATUS_FAIL;
        }
        return psa_wrappers_.psa_status_to_sl_status(status);
    }

    sl_status_t aes_cmac_compute(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t> message, std::span<uint8_t, CRYPTO_AES_CMAC_OUTPUT_LEN> mac)
    {
        return Crypto::instance().aes_cmac_compute(key, message, mac);
    }

}  // namespace crypto

extern "C" {

sl_status_t crypto_aes_cmac_compute(const uint8_t *key, const uint8_t *message, size_t message_length, uint8_t *mac)
{
    return crypto::aes_cmac_compute(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE), std::span<const uint8_t>(message, message_length), std::span<uint8_t, CRYPTO_AES_CMAC_OUTPUT_LEN>(mac, CRYPTO_AES_CMAC_OUTPUT_LEN));
}

}  // extern "C"
