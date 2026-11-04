#include "crypto_impl.hpp"

#include "psa/util.hpp"
#include "psa/psa_resources.hpp"

namespace crypto
{

    namespace
    {
        constexpr psa_key_type_t x25519_key_type = PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY);

        static_assert(crypto::x25519_private_key_size == PSA_BITS_TO_BYTES(crypto::x25519_key_bits));
        static_assert(crypto::x25519_public_key_size == PSA_BITS_TO_BYTES(crypto::x25519_key_bits));
        static_assert(crypto::x25519_shared_secret_size == PSA_RAW_KEY_AGREEMENT_OUTPUT_SIZE(x25519_key_type, crypto::x25519_key_bits));
    }  // namespace

    sl_status_t Crypto::x25519_derive_public_key(std::span<const uint8_t, crypto::x25519_private_key_size> private_key, std::span<uint8_t, crypto::x25519_public_key_size> public_key) const
    {
        detail::ScopedKey imported_key(psa_wrappers_);
        size_t public_key_len;
        psa_status_t status;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = imported_key.import_key(PSA_KEY_USAGE_DERIVE, PSA_ALG_ECDH, private_key, x25519_key_type, crypto::x25519_key_bits);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.export_public_key(imported_key.id(), public_key, public_key_len);
        if (status == PSA_SUCCESS && public_key_len != crypto::x25519_public_key_size) {
            return SL_STATUS_FAIL;
        }
        return psa_status_to_sl_status(status);
    }

    sl_status_t Crypto::x25519_derive_shared_secret(std::span<const uint8_t, crypto::x25519_private_key_size> private_key, std::span<const uint8_t, crypto::x25519_public_key_size> peer_public_key, std::span<uint8_t, crypto::x25519_shared_secret_size> shared_secret) const
    {
        detail::ScopedKey imported_key(psa_wrappers_);
        size_t shared_secret_len;
        psa_status_t status;

        status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = imported_key.import_key(PSA_KEY_USAGE_DERIVE, PSA_ALG_ECDH, private_key, x25519_key_type, crypto::x25519_key_bits);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.raw_key_agreement(PSA_ALG_ECDH, imported_key.id(), peer_public_key, shared_secret, shared_secret_len);
        if (status == PSA_SUCCESS && shared_secret_len != crypto::x25519_shared_secret_size) {
            return SL_STATUS_FAIL;
        }
        return psa_status_to_sl_status(status);
    }

}  // namespace crypto
