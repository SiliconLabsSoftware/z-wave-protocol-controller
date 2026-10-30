#ifndef AES_H_
#define AES_H_

#include "psa/crypto.h"

#define CRYPTO_AES_KEY_BITS 128U
#define CRYPTO_AES_KEY_SIZE PSA_BITS_TO_BYTES(CRYPTO_AES_KEY_BITS)

// Evaluate the PSA size expression once as a compile-time constant.
enum { CRYPTO_AES_BLOCK_SIZE = PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES) };

#endif
