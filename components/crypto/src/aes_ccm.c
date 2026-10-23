#include "aes_ccm.h"
#include "helpers.h"

#include "psa/crypto.h"

sl_status_t crypto_aes_ccm_encrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, const uint8_t *plaintext, size_t plaintext_len, uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_len)
{
    psa_key_id_t key_id;
    const psa_algorithm_t algorithm = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, tag_len);
    psa_status_t status;

    status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = import_aes_key(PSA_KEY_USAGE_ENCRYPT, algorithm, key, &key_id);
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = psa_aead_encrypt(key_id, algorithm, nonce, nonce_len, aad, aad_len, plaintext, plaintext_len, ciphertext, ciphertext_size, ciphertext_len);
    psa_destroy_key(key_id);
    return psa_status_to_sl_status(status);
}

sl_status_t crypto_aes_ccm_decrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, const uint8_t *ciphertext, size_t ciphertext_len, uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_len)
{
    psa_key_id_t key_id;
    const psa_algorithm_t algorithm = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, tag_len);
    psa_status_t status;

    status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = import_aes_key(PSA_KEY_USAGE_DECRYPT, algorithm, key, &key_id);
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = psa_aead_decrypt(key_id, algorithm, nonce, nonce_len, aad, aad_len, ciphertext, ciphertext_len, plaintext, plaintext_size, plaintext_len);
    psa_destroy_key(key_id);
    return psa_status_to_sl_status(status);
}
