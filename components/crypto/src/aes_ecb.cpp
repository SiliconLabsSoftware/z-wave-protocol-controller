#include "aes_ecb.hpp"

#include "psa/crypto.h"

namespace crypto
{

    sl_status_t Crypto::aes_ecb_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> input, std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE> output) const
    {
        psa_key_id_t key_id;
        psa_status_t status;
        size_t out_len;

        status = psa_crypto_init();
        if (status != PSA_SUCCESS) {
            return helpers_.psa_status_to_sl_status(status);
        }

        status = helpers_.import_aes_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key.data(), &key_id);
        if (status != PSA_SUCCESS) {
            return helpers_.psa_status_to_sl_status(status);
        }

        status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, input.data(), CRYPTO_AES_BLOCK_SIZE, output.data(), CRYPTO_AES_BLOCK_SIZE, &out_len);
        psa_destroy_key(key_id);
        if (status == PSA_SUCCESS && out_len != CRYPTO_AES_BLOCK_SIZE) {
            return SL_STATUS_FAIL;
        }
        return helpers_.psa_status_to_sl_status(status);
    }

    sl_status_t Crypto::aes_ecb_decrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> input, std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE> output) const
    {
        psa_key_id_t key_id;
        psa_status_t status;
        size_t out_len;

        status = psa_crypto_init();
        if (status != PSA_SUCCESS) {
            return helpers_.psa_status_to_sl_status(status);
        }

        status = helpers_.import_aes_key(PSA_KEY_USAGE_DECRYPT, PSA_ALG_ECB_NO_PADDING, key.data(), &key_id);
        if (status != PSA_SUCCESS) {
            return helpers_.psa_status_to_sl_status(status);
        }

        status = psa_cipher_decrypt(key_id, PSA_ALG_ECB_NO_PADDING, input.data(), CRYPTO_AES_BLOCK_SIZE, output.data(), CRYPTO_AES_BLOCK_SIZE, &out_len);
        psa_destroy_key(key_id);
        if (status == PSA_SUCCESS && out_len != CRYPTO_AES_BLOCK_SIZE) {
            return SL_STATUS_FAIL;
        }
        return helpers_.psa_status_to_sl_status(status);
    }

    sl_status_t aes_ecb_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> input, std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE> output)
    {
        return Crypto::instance().aes_ecb_encrypt(key, input, output);
    }

#ifdef UNIT_TEST
    sl_status_t aes_ecb_decrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> input, std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE> output)
    {
        return Crypto::instance().aes_ecb_decrypt(key, input, output);
    }
#endif

}  // namespace crypto

extern "C" {

sl_status_t crypto_aes_ecb_encrypt(const uint8_t *key, const uint8_t *input, uint8_t *output)
{
    return crypto::aes_ecb_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE), std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>(input, CRYPTO_AES_BLOCK_SIZE), std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE>(output, CRYPTO_AES_BLOCK_SIZE));
}

#ifdef UNIT_TEST
sl_status_t crypto_aes_ecb_decrypt(const uint8_t *key, const uint8_t *input, uint8_t *output)
{
    return crypto::aes_ecb_decrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE), std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>(input, CRYPTO_AES_BLOCK_SIZE), std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE>(output, CRYPTO_AES_BLOCK_SIZE));
}
#endif

}  // extern "C"
