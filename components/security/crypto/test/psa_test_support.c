#include "psa_test_support.h"

#include "psa/crypto.h"
#include "psa/crypto_extra.h"

static psa_algorithm_t ccm_alg(size_t tag_len)
{
    return PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, tag_len);
}

static psa_status_t import_aes128_key(psa_key_usage_t usage, psa_algorithm_t alg, const uint8_t *key, psa_key_id_t *key_id)
{
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

    psa_set_key_usage_flags(&attributes, usage);
    psa_set_key_algorithm(&attributes, alg);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, 128);

    return psa_import_key(&attributes, key, 16, key_id);
}

void s2_psa_test_setUp(void)
{
    (void)psa_crypto_init();
}

void s2_psa_test_tearDown(void)
{
    mbedtls_psa_crypto_free();
}

psa_status_t s2_psa_aes128_ecb_encrypt(const uint8_t *key, const uint8_t *input, uint8_t *output)
{
    psa_key_id_t key_id;
    psa_status_t status;
    size_t out_len;

    status = import_aes128_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key, &key_id);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, input, 16, output, 16, &out_len);
    psa_destroy_key(key_id);
    if (status == PSA_SUCCESS && out_len != 16) {
        return PSA_ERROR_GENERIC_ERROR;
    }
    return status;
}

psa_status_t s2_psa_aes128_ecb_decrypt(const uint8_t *key, const uint8_t *input, uint8_t *output)
{
    psa_key_id_t key_id;
    psa_status_t status;
    size_t out_len;

    status = import_aes128_key(PSA_KEY_USAGE_DECRYPT, PSA_ALG_ECB_NO_PADDING, key, &key_id);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = psa_cipher_decrypt(key_id, PSA_ALG_ECB_NO_PADDING, input, 16, output, 16, &out_len);
    psa_destroy_key(key_id);
    if (status == PSA_SUCCESS && out_len != 16) {
        return PSA_ERROR_GENERIC_ERROR;
    }
    return status;
}

psa_status_t s2_psa_aes_cmac(const uint8_t *key, const uint8_t *message, size_t message_length, uint8_t *mac)
{
    psa_key_id_t key_id;
    psa_status_t status;
    size_t mac_len;

    status = import_aes128_key(PSA_KEY_USAGE_SIGN_MESSAGE, PSA_ALG_CMAC, key, &key_id);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = psa_mac_compute(key_id, PSA_ALG_CMAC, message, message_length, mac, 16, &mac_len);
    psa_destroy_key(key_id);
    if (status == PSA_SUCCESS && mac_len != 16) {
        return PSA_ERROR_GENERIC_ERROR;
    }
    return status;
}

psa_status_t s2_psa_ccm_encrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, const uint8_t *plaintext, size_t plaintext_len, uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_len)
{
    psa_key_id_t key_id;
    psa_status_t status;
    const psa_algorithm_t alg = ccm_alg(tag_len);

    status = import_aes128_key(PSA_KEY_USAGE_ENCRYPT, alg, key, &key_id);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = psa_aead_encrypt(key_id, alg, nonce, nonce_len, aad, aad_len, plaintext, plaintext_len, ciphertext, ciphertext_size, ciphertext_len);
    psa_destroy_key(key_id);
    return status;
}

psa_status_t s2_psa_ccm_decrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, uint8_t *ciphertext, size_t ciphertext_len, uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_len)
{
    psa_key_id_t key_id;
    psa_status_t status;
    const psa_algorithm_t alg = ccm_alg(tag_len);

    status = import_aes128_key(PSA_KEY_USAGE_DECRYPT, alg, key, &key_id);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = psa_aead_decrypt(key_id, alg, nonce, nonce_len, aad, aad_len, ciphertext, ciphertext_len, plaintext, plaintext_size, plaintext_len);
    psa_destroy_key(key_id);
    return status;
}

static psa_status_t import_x25519_private(const uint8_t *private_key, psa_key_id_t *key_id)
{
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
    psa_set_key_bits(&attributes, 255);

    return psa_import_key(&attributes, private_key, 32, key_id);
}

psa_status_t s2_psa_x25519_public_from_private(const uint8_t *private_key, uint8_t *public_key)
{
    psa_key_id_t key_id;
    psa_status_t status;
    size_t pub_len;

    status = import_x25519_private(private_key, &key_id);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = psa_export_public_key(key_id, public_key, 32, &pub_len);
    psa_destroy_key(key_id);
    if (status == PSA_SUCCESS && pub_len != 32) {
        return PSA_ERROR_GENERIC_ERROR;
    }
    return status;
}

psa_status_t s2_psa_x25519_shared_secret(const uint8_t *private_key, const uint8_t *peer_public_key, uint8_t *shared_secret)
{
    psa_key_id_t key_id;
    psa_status_t status;
    size_t secret_len;

    status = import_x25519_private(private_key, &key_id);
    if (status != PSA_SUCCESS) {
        return status;
    }

    status = psa_raw_key_agreement(PSA_ALG_ECDH, key_id, peer_public_key, 32, shared_secret, 32, &secret_len);
    psa_destroy_key(key_id);
    if (status == PSA_SUCCESS && secret_len != 32) {
        return PSA_ERROR_GENERIC_ERROR;
    }
    return status;
}
