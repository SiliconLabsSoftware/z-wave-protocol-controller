#include "s2_psa_primitives.h"

#include "psa/crypto.h"

static psa_status_t s2_psa_ensure_init(void)
{
    static int initialized;

    if (initialized) {
        return PSA_SUCCESS;
    }

    const psa_status_t status = psa_crypto_init();
    if (status == PSA_SUCCESS) {
        initialized = 1;
    }
    return status;
}

psa_status_t s2_psa_crypto_init(void)
{
    return s2_psa_ensure_init();
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

psa_status_t s2_psa_aes128_ecb_encrypt(const uint8_t *key, const uint8_t *input, uint8_t *output)
{
    psa_key_id_t key_id;
    psa_status_t status;
    size_t out_len;

    status = s2_psa_ensure_init();
    if (status != PSA_SUCCESS) {
        return status;
    }

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

psa_status_t s2_psa_aes_cmac(const uint8_t *key, const uint8_t *message, size_t message_length, uint8_t *mac)
{
    psa_key_id_t key_id;
    psa_status_t status;
    size_t mac_len;

    status = s2_psa_ensure_init();
    if (status != PSA_SUCCESS) {
        return status;
    }

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
