#ifndef AES_CMAC_HPP
#define AES_CMAC_HPP

#include "aes_cmac.h"

#include <span>

namespace crypto
{

    sl_status_t aes_cmac_compute(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, std::span<const uint8_t> message, std::span<uint8_t, CRYPTO_AES_CMAC_OUTPUT_LEN> mac);

}  // namespace crypto

#endif
