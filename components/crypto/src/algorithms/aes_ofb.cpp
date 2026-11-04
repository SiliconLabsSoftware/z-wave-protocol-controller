#include "crypto_impl.hpp"

#include "psa/crypto.h"
#include "psa/util.hpp"
#include "psa/psa_resources.hpp"

namespace crypto
{

    sl_status_t Crypto::aes_ofb_xor(std::span<const uint8_t, crypto::aes_key_size> key, std::span<const uint8_t, crypto::aes_block_size> iv, std::span<uint8_t> data) const
    {
        detail::ScopedKey imported_key(psa_wrappers_);
        detail::ScopedCipherOperation operation(psa_wrappers_);
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

        status = imported_key.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_OFB, key, PSA_KEY_TYPE_AES, crypto::aes_key_bits);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = operation.encrypt_setup(imported_key.id(), PSA_ALG_OFB);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_set_iv(operation.get(), iv);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_update(operation.get(), data, data, update_len);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = operation.finish(data.subspan(update_len), finish_len);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        if (update_len + finish_len != len) {
            return SL_STATUS_FAIL;
        }

        return SL_STATUS_OK;
    }

}  // namespace crypto
