#ifndef AES_CBC_MAC_HPP
#define AES_CBC_MAC_HPP

#include "aes_cbc_mac.h"
#include "crypto.hpp"

#include <span>

namespace crypto
{

    sl_status_t aes_cbc_mac(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE> iv, std::span<const uint8_t> data, std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE> mac);

}  // namespace crypto

#endif
