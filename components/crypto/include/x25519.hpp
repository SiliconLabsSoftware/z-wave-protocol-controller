#ifndef X25519_HPP
#define X25519_HPP

#include "x25519.h"

#include <span>

namespace crypto
{

    sl_status_t x25519_derive_public_key(std::span<const uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE> private_key, std::span<uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE> public_key);

    sl_status_t x25519_derive_shared_secret(std::span<const uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE> private_key, std::span<const uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE> peer_public_key, std::span<uint8_t, CRYPTO_X25519_SHARED_SECRET_SIZE> shared_secret);

}  // namespace crypto

#endif
