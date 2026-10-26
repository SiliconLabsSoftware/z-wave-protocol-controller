#include "psa_test_support.hpp"

#include "psa/crypto.h"
#include "psa/crypto_extra.h"

sl_status_t aes128_ecb_decrypt(const uint8_t *key, const uint8_t *input, uint8_t *output)
{
    psa_key_id_t key_id;
    psa_status_t status;
    size_t out_len;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

    status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        return SL_STATUS_FAIL;
    }

    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECB_NO_PADDING);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, 128);

    status = psa_import_key(&attributes, key, 16, &key_id);
    if (status != PSA_SUCCESS) {
        return SL_STATUS_FAIL;
    }

    status = psa_cipher_decrypt(key_id, PSA_ALG_ECB_NO_PADDING, input, 16, output, 16, &out_len);
    psa_destroy_key(key_id);
    if (status == PSA_SUCCESS && out_len != 16) {
        return SL_STATUS_FAIL;
    }
    return (status == PSA_SUCCESS) ? SL_STATUS_OK : SL_STATUS_FAIL;
}
