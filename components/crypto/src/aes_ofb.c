#include "aes_ofb.h"
#include "aes.h"
#include "helpers.h"

#include "psa/crypto.h"

sl_status_t crypto_aes_ofb_encrypt(const uint8_t *key, const uint8_t *iv, uint8_t *data, size_t data_len)
{
    if (key == NULL || iv == NULL || (data_len != 0 && data == NULL)) {
        return SL_STATUS_INVALID_PARAMETER;
    }
    if (data_len == 0) {
        return SL_STATUS_OK;
    }

    psa_key_id_t key_id;
    psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
    psa_status_t status;
    size_t update_len;
    size_t finish_len;

    status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = import_aes_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_OFB, key, &key_id);
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = psa_cipher_encrypt_setup(&operation, key_id, PSA_ALG_OFB);
    if (status != PSA_SUCCESS) {
        psa_destroy_key(key_id);
        return psa_status_to_sl_status(status);
    }

    status = psa_cipher_set_iv(&operation, iv, CRYPTO_AES_BLOCK_SIZE);
    if (status != PSA_SUCCESS) {
        psa_cipher_abort(&operation);
        psa_destroy_key(key_id);
        return psa_status_to_sl_status(status);
    }

    status = psa_cipher_update(&operation, data, data_len, data, data_len, &update_len);
    if (status != PSA_SUCCESS) {
        psa_cipher_abort(&operation);
        psa_destroy_key(key_id);
        return psa_status_to_sl_status(status);
    }

    status = psa_cipher_finish(&operation, data + update_len, data_len - update_len, &finish_len);
    if (status != PSA_SUCCESS) {
        psa_cipher_abort(&operation);
        psa_destroy_key(key_id);
        return psa_status_to_sl_status(status);
    }

    psa_destroy_key(key_id);

    if (update_len + finish_len != data_len) {
        return SL_STATUS_FAIL;
    }

    return SL_STATUS_OK;
}
