#include "aes_ofb.hpp"

#include "helpers.hpp"

#include "psa/crypto.h"

namespace
{

    psa_status_t import_aes_ofb_key(const uint8_t *key, psa_key_id_t *key_id)
    {
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
        psa_set_key_algorithm(&attributes, PSA_ALG_OFB);
        psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
        psa_set_key_bits(&attributes, CRYPTO_AES_KEY_BITS);

        return psa_import_key(&attributes, key, CRYPTO_AES_KEY_SIZE, key_id);
    }

}  // namespace

namespace crypto
{

    sl_status_t aes_ofb_xor(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> iv, std::span<uint8_t> data)
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

        status = psa_crypto_init();
        if (status != PSA_SUCCESS) {
            return helpers::psa_status_to_sl_status(status);
        }

        status = import_aes_ofb_key(key.data(), &key_id);
        if (status != PSA_SUCCESS) {
            return helpers::psa_status_to_sl_status(status);
        }

        status = psa_cipher_encrypt_setup(&operation, key_id, PSA_ALG_OFB);
        if (status != PSA_SUCCESS) {
            psa_destroy_key(key_id);
            return helpers::psa_status_to_sl_status(status);
        }

        status = psa_cipher_set_iv(&operation, iv.data(), CRYPTO_AES_BLOCK_SIZE);
        if (status != PSA_SUCCESS) {
            psa_cipher_abort(&operation);
            psa_destroy_key(key_id);
            return helpers::psa_status_to_sl_status(status);
        }

        status = psa_cipher_update(&operation, data.data(), len, data.data(), len, &update_len);
        if (status != PSA_SUCCESS) {
            psa_cipher_abort(&operation);
            psa_destroy_key(key_id);
            return helpers::psa_status_to_sl_status(status);
        }

        status = psa_cipher_finish(&operation, data.data() + update_len, len - update_len, &finish_len);
        if (status != PSA_SUCCESS) {
            psa_cipher_abort(&operation);
            psa_destroy_key(key_id);
            return helpers::psa_status_to_sl_status(status);
        }

        psa_destroy_key(key_id);

        if (update_len + finish_len != len) {
            return SL_STATUS_FAIL;
        }

        return SL_STATUS_OK;
    }

}  // namespace crypto

extern "C" {

sl_status_t crypto_aes_ofb_xor(const uint8_t *key, uint8_t iv[16], uint8_t *data, size_t len)
{
    if (len == 0) {
        return SL_STATUS_OK;
    }

    return crypto::aes_ofb_xor(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE), std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>(iv, CRYPTO_AES_BLOCK_SIZE), std::span<uint8_t>(data, len));
}

}  // extern "C"
