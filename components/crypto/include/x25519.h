#ifndef X25519_H_
#define X25519_H_

#include <stdint.h>

#include "psa/crypto.h"
#include "sl_status.h"

#define CRYPTO_X25519_KEY_BITS         255U
#define CRYPTO_X25519_KEY_TYPE         PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY)
#define CRYPTO_X25519_PRIVATE_KEY_SIZE PSA_BITS_TO_BYTES(CRYPTO_X25519_KEY_BITS)
#define CRYPTO_X25519_PUBLIC_KEY_SIZE  PSA_BITS_TO_BYTES(CRYPTO_X25519_KEY_BITS)

// Evaluate the PSA size expression once as a compile-time constant.
enum { CRYPTO_X25519_SHARED_SECRET_SIZE = PSA_RAW_KEY_AGREEMENT_OUTPUT_SIZE(CRYPTO_X25519_KEY_TYPE, CRYPTO_X25519_KEY_BITS) };

#ifdef __cplusplus
extern "C" {
#endif

sl_status_t crypto_x25519_derive_public_key(const uint8_t *private_key, uint8_t *public_key);
sl_status_t crypto_x25519_derive_shared_secret(const uint8_t *private_key, const uint8_t *peer_public_key, uint8_t *shared_secret);

#ifdef __cplusplus
}
#endif

#endif
