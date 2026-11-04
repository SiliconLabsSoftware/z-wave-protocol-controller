#include "crypto_impl.hpp"

#include <cstring>

#include "psa/crypto.h"
#include "psa/util.hpp"
#include "psa/psa_wrappers_interface.hpp"

namespace
{

    const uint8_t cbc_mac_zero_iv[crypto::aes_block_size] {};

}  // namespace

namespace crypto
{

    sl_status_t Crypto::aes_cbc_mac(std::span<const uint8_t, crypto::aes_key_size> key, std::span<const uint8_t, crypto::aes_block_size> iv, std::span<const uint8_t> data, std::span<uint8_t, crypto::aes_block_size> mac) const
    {
        if (data.size() > UINT8_MAX) {
            return SL_STATUS_INVALID_PARAMETER;
        }

        const uint8_t len = static_cast<uint8_t>(data.size());
        uint8_t message[((crypto::aes_block_size + UINT8_MAX + crypto::aes_block_size - 1) / crypto::aes_block_size) * crypto::aes_block_size];
        uint8_t ciphertext[sizeof(message)];
        size_t message_len;
        size_t pad_len;
        psa_key_id_t key_id;
        psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
        psa_status_t status;
        size_t update_len;
        size_t finish_len;
        size_t ciphertext_len;

        memcpy(message, iv.data(), crypto::aes_block_size);
        if (len > 0) {
            memcpy(message + crypto::aes_block_size, data.data(), len);
        }
        message_len = crypto::aes_block_size + len;
        pad_len     = (crypto::aes_block_size - (message_len % crypto::aes_block_size)) % crypto::aes_block_size;
        memset(message + message_len, 0, pad_len);
        message_len += pad_len;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, crypto::aes_key_bits);
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
        if (ciphertext_len < crypto::aes_block_size) {
            return SL_STATUS_FAIL;
        }
        memcpy(mac.data(), ciphertext + ciphertext_len - crypto::aes_block_size, crypto::aes_block_size);
        return SL_STATUS_OK;
    }

}  // namespace crypto
