#include "crypto_impl.hpp"

#include "psa/crypto.h"
#include "psa/util.hpp"
#include "psa/psa_wrappers_interface.hpp"

namespace crypto
{

    sl_status_t Crypto::aes_cmac_compute(std::span<const uint8_t, crypto::aes_key_size> key, std::span<const uint8_t> message, std::span<uint8_t, crypto::aes_cmac_output_len> mac) const
    {
        psa_key_id_t key_id;
        psa_status_t status;
        size_t mac_len;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.import_key(PSA_KEY_USAGE_SIGN_MESSAGE, PSA_ALG_CMAC, key, key_id, PSA_KEY_TYPE_AES, crypto::aes_key_bits);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.mac_compute(key_id, PSA_ALG_CMAC, message, mac, mac_len);
        psa_wrappers_.destroy_key(key_id);
        if (status == PSA_SUCCESS && mac_len != crypto::aes_cmac_output_len) {
            return SL_STATUS_FAIL;
        }
        return psa_status_to_sl_status(status);
    }

}  // namespace crypto
