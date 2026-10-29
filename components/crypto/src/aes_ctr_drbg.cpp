#include <cstring>
#include <stdint.h>

#include "aes_ctr_drbg.hpp"
#include "aes_ecb.hpp"

namespace
{

    sl_status_t increment(uint8_t *data, uint8_t length)
    {
        if (length == 0) {
            return SL_STATUS_INVALID_PARAMETER;
        }

        while ((length--) != 0U) {
            data[length]++;
            if (data[length] != 0U) {
                break;
            }
        }

        return SL_STATUS_OK;
    }

}  // namespace

namespace crypto
{

    sl_status_t Crypto::update_ctr_drbg(crypto_aes_ctr_drbg_ctx_t &ctx, const uint8_t *data) const
    {
        uint8_t tmp[CRYPTO_AES_CTR_DRBG_SEED_SIZE] = {0};

        for (uint8_t i = 0; i < CRYPTO_AES_CTR_DRBG_SEED_SIZE; i += CRYPTO_AES_BLOCK_SIZE) {
            sl_status_t status = increment(ctx.counter, CRYPTO_AES_BLOCK_SIZE);
            if (status != SL_STATUS_OK) {
                return status;
            }
            status = aes_ecb_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(ctx.key), std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>(ctx.counter), std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE>(&tmp[i], CRYPTO_AES_BLOCK_SIZE));
            if (status != SL_STATUS_OK) {
                return status;
            }
        }

        for (uint8_t i = 0; i < CRYPTO_AES_CTR_DRBG_SEED_SIZE; i++) {
            tmp[i] ^= data[i];
        }

        memcpy(ctx.key, tmp, CRYPTO_AES_KEY_SIZE);
        memcpy(ctx.counter, tmp + CRYPTO_AES_KEY_SIZE, CRYPTO_AES_BLOCK_SIZE);
        return SL_STATUS_OK;
    }

    sl_status_t Crypto::aes_ctr_drbg_init(crypto_aes_ctr_drbg_ctx_t &ctx, std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> entropy_input, std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> pers_string) const
    {
        crypto_aes_ctr_drbg_ctx_t next_ctx = {};
        uint8_t seed[CRYPTO_AES_CTR_DRBG_SEED_SIZE];

        memcpy(seed, entropy_input.data(), sizeof(seed));
        for (uint8_t i = 0; i < CRYPTO_AES_CTR_DRBG_SEED_SIZE; i++) {
            seed[i] ^= pers_string[i];
        }

        sl_status_t status = update_ctr_drbg(next_ctx, seed);
        if (status != SL_STATUS_OK) {
            return status;
        }

        ctx = next_ctx;
        return SL_STATUS_OK;
    }

    sl_status_t Crypto::aes_ctr_drbg_generate(crypto_aes_ctr_drbg_ctx_t &ctx, std::span<uint8_t> out) const
    {
        if (out.data() == nullptr || out.size() != CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE) {
            return SL_STATUS_INVALID_PARAMETER;
        }

        crypto_aes_ctr_drbg_ctx_t next_ctx = ctx;
        uint8_t block[CRYPTO_AES_BLOCK_SIZE];
        uint8_t provided_data[CRYPTO_AES_CTR_DRBG_SEED_SIZE] = {0};
        uint8_t generated[CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE];
        uint8_t *generated_pos = generated;
        size_t copy;
        size_t remaining = CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE;

        while (remaining != 0U) {
            sl_status_t status = increment(next_ctx.counter, CRYPTO_AES_BLOCK_SIZE);
            if (status != SL_STATUS_OK) {
                return status;
            }
            status = aes_ecb_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(next_ctx.key), std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>(next_ctx.counter), std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE>(block));
            if (status != SL_STATUS_OK) {
                return status;
            }
            copy = (remaining < CRYPTO_AES_BLOCK_SIZE) ? remaining : CRYPTO_AES_BLOCK_SIZE;
            memcpy(generated_pos, block, copy);
            generated_pos += copy;
            remaining -= copy;
        }

        memset(provided_data, 0, sizeof(provided_data));
        sl_status_t status = update_ctr_drbg(next_ctx, provided_data);
        if (status != SL_STATUS_OK) {
            return status;
        }

        ctx = next_ctx;
        memcpy(out.data(), generated, sizeof(generated));
        return SL_STATUS_OK;
    }

    sl_status_t aes_ctr_drbg_init(crypto_aes_ctr_drbg_ctx_t &ctx, std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> entropy_input, std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> pers_string)
    {
        return Crypto::instance().aes_ctr_drbg_init(ctx, entropy_input, pers_string);
    }

    sl_status_t aes_ctr_drbg_generate(crypto_aes_ctr_drbg_ctx_t &ctx, std::span<uint8_t, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE> out)
    {
        return Crypto::instance().aes_ctr_drbg_generate(ctx, out);
    }

}  // namespace crypto

extern "C" {

sl_status_t crypto_aes_ctr_drbg_init(crypto_aes_ctr_drbg_ctx_t *ctx, const uint8_t *entropy_input, const uint8_t *pers_string)
{
    if (ctx == NULL || entropy_input == NULL || pers_string == NULL) {
        return SL_STATUS_INVALID_PARAMETER;
    }

    return crypto::aes_ctr_drbg_init(*ctx, std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE>(entropy_input, CRYPTO_AES_CTR_DRBG_SEED_SIZE), std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE>(pers_string, CRYPTO_AES_CTR_DRBG_SEED_SIZE));
}

sl_status_t crypto_aes_ctr_drbg_generate(crypto_aes_ctr_drbg_ctx_t *ctx, uint8_t *out)
{
    if (ctx == NULL || out == NULL) {
        return SL_STATUS_INVALID_PARAMETER;
    }

    return crypto::aes_ctr_drbg_generate(*ctx, std::span<uint8_t, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE>(out, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE));
}

}  // extern "C"
