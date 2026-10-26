#ifndef AES_ECB_HPP
#define AES_ECB_HPP

#include "aes_ecb.h"

#include <span>

namespace crypto
{

    sl_status_t aes_ecb_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> input, std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE> output);

}  // namespace crypto

#endif
