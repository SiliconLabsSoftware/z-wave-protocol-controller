#include "crypto.hpp"

#include <cstring>

#include "psa/crypto.h"
#include "psa_status.hpp"
#include "psa_wrappers_interface.hpp"

namespace
{

    const uint8_t cbc_mac_zero_iv[CRYPTO_AES_BLOCK_SIZE] {};

}  // namespace

namespace crypto
{

    sl_status_t Crypto::aes_cbc_mac(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> iv, std::span<const uint8_t> data, std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE> mac) const
    {
        if (data.size() > UINT8_MAX) {
            return SL_STATUS_INVALID_PARAMETER;
        }

        const uint8_t len = static_cast<uint8_t>(data.size());
        uint8_t message[((CRYPTO_AES_BLOCK_SIZE + UINT8_MAX + CRYPTO_AES_BLOCK_SIZE - 1) / CRYPTO_AES_BLOCK_SIZE) * CRYPTO_AES_BLOCK_SIZE];
        uint8_t ciphertext[sizeof(message)];
        size_t message_len;
        size_t pad_len;
        psa_key_id_t key_id;
        psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
        psa_status_t status;
        size_t update_len;
        size_t finish_len;
        size_t ciphertext_len;

        memcpy(message, iv.data(), CRYPTO_AES_BLOCK_SIZE);
        if (len > 0) {
            memcpy(message + CRYPTO_AES_BLOCK_SIZE, data.data(), len);
        }
        message_len = CRYPTO_AES_BLOCK_SIZE + len;
        pad_len     = (CRYPTO_AES_BLOCK_SIZE - (message_len % CRYPTO_AES_BLOCK_SIZE)) % CRYPTO_AES_BLOCK_SIZE;
        memset(message + message_len, 0, pad_len);
        message_len += pad_len;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_encrypt_setup(operation, key_id, PSA_ALG_CBC_NO_PADDING);
        if (status != PSA_SUCCESS) {
            psa_wrappers_.destroy_key(key_id);
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_set_iv(operation, cbc_mac_zero_iv);
        if (status != PSA_SUCCESS) {
            psa_wrappers_.cipher_abort(operation);
            psa_wrappers_.destroy_key(key_id);
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_update(operation, std::span<const uint8_t>(message, message_len), ciphertext, update_len);
        if (status != PSA_SUCCESS) {
            psa_wrappers_.cipher_abort(operation);
            psa_wrappers_.destroy_key(key_id);
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_finish(operation, std::span(ciphertext).subspan(update_len), finish_len);
        if (status != PSA_SUCCESS) {
            psa_wrappers_.cipher_abort(operation);
            psa_wrappers_.destroy_key(key_id);
            return psa_status_to_sl_status(status);
        }

        psa_wrappers_.destroy_key(key_id);

        ciphertext_len = update_len + finish_len;
        if (ciphertext_len < CRYPTO_AES_BLOCK_SIZE) {
            return SL_STATUS_FAIL;
        }
        memcpy(mac.data(), ciphertext + ciphertext_len - CRYPTO_AES_BLOCK_SIZE, CRYPTO_AES_BLOCK_SIZE);
        return SL_STATUS_OK;
    }

}  // namespace crypto

extern "C" {

sl_status_t crypto_aes_cbc_mac(const uint8_t *key, const uint8_t *iv, const uint8_t *data, uint8_t len, uint8_t *mac)
{
    return crypto::Crypto::instance().aes_cbc_mac(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE),
                                                  std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>(iv, CRYPTO_AES_BLOCK_SIZE),
                                                  std::span<const uint8_t>(data, len),
                                                  std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE>(mac, CRYPTO_AES_BLOCK_SIZE));
}

}  // extern "C"
