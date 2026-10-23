#include "aes_cbc_mac.h"
#include "aes.h"
#include "helpers.h"

#include <string.h>

#include "psa/crypto.h"

static const uint8_t cbc_mac_zero_iv[CRYPTO_AES_BLOCK_SIZE];

sl_status_t crypto_aes_cbc_mac(const uint8_t *key, const uint8_t *iv, const uint8_t *data, uint8_t len, uint8_t *mac)
{
    uint8_t message[CRYPTO_AES_BLOCK_SIZE + UINT8_MAX];
    uint8_t ciphertext[sizeof(message)];
    size_t message_len;
    size_t pad_len;
    psa_key_id_t key_id;
    psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
    psa_status_t status;
    size_t update_len;
    size_t finish_len;
    size_t ciphertext_len;

    memcpy(message, iv, CRYPTO_AES_BLOCK_SIZE);
    if (len > 0) {
        memcpy(message + CRYPTO_AES_BLOCK_SIZE, data, len);
    }
    message_len = CRYPTO_AES_BLOCK_SIZE + len;
    pad_len     = (CRYPTO_AES_BLOCK_SIZE - (message_len % CRYPTO_AES_BLOCK_SIZE)) % CRYPTO_AES_BLOCK_SIZE;
    memset(message + message_len, 0, pad_len);
    message_len += pad_len;

    status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = import_aes_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING, key, &key_id);
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = psa_cipher_encrypt_setup(&operation, key_id, PSA_ALG_CBC_NO_PADDING);
    if (status != PSA_SUCCESS) {
        psa_destroy_key(key_id);
        return psa_status_to_sl_status(status);
    }

    status = psa_cipher_set_iv(&operation, cbc_mac_zero_iv, CRYPTO_AES_BLOCK_SIZE);
    if (status != PSA_SUCCESS) {
        psa_cipher_abort(&operation);
        psa_destroy_key(key_id);
        return psa_status_to_sl_status(status);
    }

    status = psa_cipher_update(&operation, message, message_len, ciphertext, sizeof(ciphertext), &update_len);
    if (status != PSA_SUCCESS) {
        psa_cipher_abort(&operation);
        psa_destroy_key(key_id);
        return psa_status_to_sl_status(status);
    }

    status = psa_cipher_finish(&operation, ciphertext + update_len, sizeof(ciphertext) - update_len, &finish_len);
    if (status != PSA_SUCCESS) {
        psa_cipher_abort(&operation);
        psa_destroy_key(key_id);
        return psa_status_to_sl_status(status);
    }

    psa_destroy_key(key_id);

    ciphertext_len = update_len + finish_len;
    if (ciphertext_len < CRYPTO_AES_BLOCK_SIZE) {
        return SL_STATUS_FAIL;
    }
    memcpy(mac, ciphertext + ciphertext_len - CRYPTO_AES_BLOCK_SIZE, CRYPTO_AES_BLOCK_SIZE);
    return SL_STATUS_OK;
}
