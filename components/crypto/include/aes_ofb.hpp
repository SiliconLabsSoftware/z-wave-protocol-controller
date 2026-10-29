#ifndef AES_OFB_HPP
#define AES_OFB_HPP

#include "aes_ofb.h"
#include "crypto.hpp"

#include <span>

namespace crypto
{

    sl_status_t aes_ofb_xor(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> iv, std::span<uint8_t> data);

}  // namespace crypto

#endif
