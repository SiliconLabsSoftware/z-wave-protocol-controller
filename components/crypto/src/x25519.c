#include "x25519.h"
#include "helpers.h"

static psa_status_t import_x25519_private_key(const uint8_t *private_key, psa_key_id_t *key_id)
{
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);
    psa_set_key_type(&attributes, CRYPTO_X25519_KEY_TYPE);
    psa_set_key_bits(&attributes, CRYPTO_X25519_KEY_BITS);

    return psa_import_key(&attributes, private_key, CRYPTO_X25519_PRIVATE_KEY_SIZE, key_id);
}

sl_status_t crypto_x25519_derive_public_key(const uint8_t *private_key, uint8_t *public_key)
{
    psa_key_id_t key_id;
    size_t public_key_len;
    psa_status_t status;

    status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = import_x25519_private_key(private_key, &key_id);
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = psa_export_public_key(key_id, public_key, CRYPTO_X25519_PUBLIC_KEY_SIZE, &public_key_len);
    psa_destroy_key(key_id);
    if (status == PSA_SUCCESS && public_key_len != CRYPTO_X25519_PUBLIC_KEY_SIZE) {
        return SL_STATUS_FAIL;
    }
    return psa_status_to_sl_status(status);
}

sl_status_t crypto_x25519_derive_shared_secret(const uint8_t *private_key, const uint8_t *peer_public_key, uint8_t *shared_secret)
{
    psa_key_id_t key_id;
    size_t shared_secret_len;
    psa_status_t status;

    status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = import_x25519_private_key(private_key, &key_id);
    if (status != PSA_SUCCESS) {
        return psa_status_to_sl_status(status);
    }

    status = psa_raw_key_agreement(PSA_ALG_ECDH, key_id, peer_public_key, CRYPTO_X25519_PUBLIC_KEY_SIZE, shared_secret, CRYPTO_X25519_SHARED_SECRET_SIZE, &shared_secret_len);
    psa_destroy_key(key_id);
    if (status == PSA_SUCCESS && shared_secret_len != CRYPTO_X25519_SHARED_SECRET_SIZE) {
        return SL_STATUS_FAIL;
    }
    return psa_status_to_sl_status(status);
}
