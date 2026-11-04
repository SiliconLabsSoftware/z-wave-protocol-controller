#include "crypto_impl.hpp"

#include "psa/crypto.h"
#include "psa/util.hpp"
#include "psa/psa_wrappers_interface.hpp"

namespace crypto
{

    static_assert(crypto::aes_key_size == PSA_BITS_TO_BYTES(crypto::aes_key_bits));
    static_assert(crypto::aes_block_size == PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES));

    sl_status_t Crypto::aes_ecb_encrypt(std::span<const uint8_t, crypto::aes_key_size> key, std::span<const uint8_t, crypto::aes_block_size> input, std::span<uint8_t, crypto::aes_block_size> output) const
    {
        psa_key_id_t key_id;
        psa_status_t status;
        size_t out_len;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, crypto::aes_key_bits);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, input, output, out_len);
        psa_wrappers_.destroy_key(key_id);
        if (status == PSA_SUCCESS && out_len != crypto::aes_block_size) {
            return SL_STATUS_FAIL;
        }
        return psa_status_to_sl_status(status);
    }

    sl_status_t Crypto::aes_ecb_decrypt(std::span<const uint8_t, crypto::aes_key_size> key, std::span<const uint8_t, crypto::aes_block_size> input, std::span<uint8_t, crypto::aes_block_size> output) const
    {
        psa_key_id_t key_id;
        psa_status_t status;
        size_t out_len;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.import_key(PSA_KEY_USAGE_DECRYPT, PSA_ALG_ECB_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, crypto::aes_key_bits);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_decrypt(key_id, PSA_ALG_ECB_NO_PADDING, input, output, out_len);
        psa_wrappers_.destroy_key(key_id);
        if (status == PSA_SUCCESS && out_len != crypto::aes_block_size) {
            return SL_STATUS_FAIL;
        }
        return psa_status_to_sl_status(status);
    }

}  // namespace crypto
