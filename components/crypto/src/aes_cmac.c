#include "aes_cmac.h"
#include "helpers.h"

#include "psa/crypto.h"

sl_status_t crypto_aes_cmac_compute(const uint8_t *key, const uint8_t *message, size_t message_length, uint8_t *mac)
{
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id;
    psa_status_t status;
    size_t mac_len;

    status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_algorithm(&attributes, PSA_ALG_CMAC);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, CRYPTO_AES_KEY_BITS);

    status = psa_import_key(&attributes, key, CRYPTO_AES_KEY_SIZE, &key_id);
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = psa_mac_compute(key_id, PSA_ALG_CMAC, message, message_length, mac, CRYPTO_AES_CMAC_OUTPUT_LEN, &mac_len);
    psa_destroy_key(key_id);
    if (status == PSA_SUCCESS && mac_len != CRYPTO_AES_CMAC_OUTPUT_LEN) {
        return SL_STATUS_FAIL;
    }
    return psa_status_to_sl_status(status);
}
