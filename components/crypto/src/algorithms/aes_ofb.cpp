#include "crypto_impl.hpp"

#include "psa/crypto.h"
#include "psa/util.hpp"
#include "psa/psa_wrappers_interface.hpp"

namespace crypto
{

    sl_status_t Crypto::aes_ofb_xor(std::span<const uint8_t, crypto::aes_key_size> key, std::span<const uint8_t, crypto::aes_block_size> iv, std::span<uint8_t> data) const
    {
        psa_key_id_t key_id;
        psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
        psa_status_t status;
        size_t update_len;
        size_t finish_len;
        const size_t len = data.size();

        if (len == 0) {
            return SL_STATUS_OK;
        }

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_OFB, key, key_id, PSA_KEY_TYPE_AES, crypto::aes_key_bits);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_encrypt_setup(operation, key_id, PSA_ALG_OFB);
        if (status != PSA_SUCCESS) {
            psa_wrappers_.destroy_key(key_id);
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_set_iv(operation, iv);
        if (status != PSA_SUCCESS) {
            psa_wrappers_.cipher_abort(operation);
            psa_wrappers_.destroy_key(key_id);
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_update(operation, data, data, update_len);
        if (status != PSA_SUCCESS) {
            psa_wrappers_.cipher_abort(operation);
            psa_wrappers_.destroy_key(key_id);
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_finish(operation, data.subspan(update_len), finish_len);
        if (status != PSA_SUCCESS) {
            psa_wrappers_.cipher_abort(operation);
            psa_wrappers_.destroy_key(key_id);
            return psa_status_to_sl_status(status);
        }

        psa_wrappers_.destroy_key(key_id);

        if (update_len + finish_len != len) {
            return SL_STATUS_FAIL;
        }

        return SL_STATUS_OK;
    }

}  // namespace crypto
