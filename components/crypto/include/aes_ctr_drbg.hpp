#ifndef AES_CTR_DRBG_HPP
#define AES_CTR_DRBG_HPP

#include "aes_ctr_drbg.h"

#include <span>

namespace crypto
{

    sl_status_t aes_ctr_drbg_init(crypto_aes_ctr_drbg_ctx_t &ctx, std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> entropy_input, std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> pers_string);

    sl_status_t aes_ctr_drbg_generate(crypto_aes_ctr_drbg_ctx_t &ctx, std::span<uint8_t, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE> out);

}  // namespace crypto

#endif
