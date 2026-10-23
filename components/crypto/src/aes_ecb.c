#include "aes_ecb.h"
#include "aes.h"
#include "helpers.h"

#include "psa/crypto.h"

sl_status_t crypto_aes_ecb_encrypt(const uint8_t *key, const uint8_t *input, uint8_t *output)
{
    psa_key_id_t key_id;
    psa_status_t status;
    size_t out_len;

    status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = import_aes_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key, &key_id);
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, input, CRYPTO_AES_BLOCK_SIZE, output, CRYPTO_AES_BLOCK_SIZE, &out_len);
    psa_destroy_key(key_id);
    if (status == PSA_SUCCESS && out_len != CRYPTO_AES_BLOCK_SIZE) {
        return SL_STATUS_FAIL;
    }
    return psa_status_to_sl_status(status);
}
