#include "crypto.hpp"

#include "util.hpp"
#include "psa_wrappers_interface.hpp"

namespace crypto
{

    namespace
    {
        constexpr psa_key_type_t x25519_key_type = PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY);

        static_assert(CRYPTO_X25519_PRIVATE_KEY_SIZE == PSA_BITS_TO_BYTES(CRYPTO_X25519_KEY_BITS));
        static_assert(CRYPTO_X25519_PUBLIC_KEY_SIZE == PSA_BITS_TO_BYTES(CRYPTO_X25519_KEY_BITS));
        static_assert(CRYPTO_X25519_SHARED_SECRET_SIZE == PSA_RAW_KEY_AGREEMENT_OUTPUT_SIZE(x25519_key_type, CRYPTO_X25519_KEY_BITS));
    }  // namespace

    sl_status_t Crypto::x25519_derive_public_key(std::span<const uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE> private_key, std::span<uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE> public_key) const
    {
        psa_key_id_t key_id;
        size_t public_key_len;
        psa_status_t status;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.import_key(PSA_KEY_USAGE_DERIVE, PSA_ALG_ECDH, private_key, key_id, x25519_key_type, CRYPTO_X25519_KEY_BITS);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.export_public_key(key_id, public_key, public_key_len);
        psa_wrappers_.destroy_key(key_id);
        if (status == PSA_SUCCESS && public_key_len != CRYPTO_X25519_PUBLIC_KEY_SIZE) {
            return SL_STATUS_FAIL;
        }
        return psa_status_to_sl_status(status);
    }

    sl_status_t Crypto::x25519_derive_shared_secret(std::span<const uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE> private_key, std::span<const uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE> peer_public_key, std::span<uint8_t, CRYPTO_X25519_SHARED_SECRET_SIZE> shared_secret) const
    {
        psa_key_id_t key_id;
        size_t shared_secret_len;
        psa_status_t status;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.import_key(PSA_KEY_USAGE_DERIVE, PSA_ALG_ECDH, private_key, key_id, x25519_key_type, CRYPTO_X25519_KEY_BITS);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.raw_key_agreement(PSA_ALG_ECDH, key_id, peer_public_key, shared_secret, shared_secret_len);
        psa_wrappers_.destroy_key(key_id);
        if (status == PSA_SUCCESS && shared_secret_len != CRYPTO_X25519_SHARED_SECRET_SIZE) {
            return SL_STATUS_FAIL;
        }
        return psa_status_to_sl_status(status);
    }

}  // namespace crypto

extern "C" {

sl_status_t crypto_x25519_derive_public_key(const uint8_t *private_key, uint8_t *public_key)
{
    return crypto::c_api().x25519_derive_public_key(std::span<const uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE>(private_key, CRYPTO_X25519_PRIVATE_KEY_SIZE), std::span<uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE>(public_key, CRYPTO_X25519_PUBLIC_KEY_SIZE));
}

sl_status_t crypto_x25519_derive_shared_secret(const uint8_t *private_key, const uint8_t *peer_public_key, uint8_t *shared_secret)
{
    return crypto::c_api().x25519_derive_shared_secret(std::span<const uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE>(private_key, CRYPTO_X25519_PRIVATE_KEY_SIZE),
                                                       std::span<const uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE>(peer_public_key, CRYPTO_X25519_PUBLIC_KEY_SIZE),
                                                       std::span<uint8_t, CRYPTO_X25519_SHARED_SECRET_SIZE>(shared_secret, CRYPTO_X25519_SHARED_SECRET_SIZE));
}

}  // extern "C"
