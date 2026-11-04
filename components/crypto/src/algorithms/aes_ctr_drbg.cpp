#include <cstring>
#include <stdint.h>

#include "crypto_impl.hpp"

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

    sl_status_t Crypto::update_ctr_drbg(crypto::CtrDrbgContext &ctx, const uint8_t *data) const
    {
        uint8_t tmp[crypto::aes_ctr_drbg_seed_size] = {0};

        for (uint8_t i = 0; i < crypto::aes_ctr_drbg_seed_size; i += crypto::aes_block_size) {
            sl_status_t status = increment(ctx.counter.data(), crypto::aes_block_size);
            if (status != SL_STATUS_OK) {
                return status;
            }
            status = aes_ecb_encrypt(std::span<const uint8_t, crypto::aes_key_size>(ctx.key), std::span<const uint8_t, crypto::aes_block_size>(ctx.counter), std::span<uint8_t, crypto::aes_block_size>(&tmp[i], crypto::aes_block_size));
            if (status != SL_STATUS_OK) {
                return status;
            }
        }

        for (uint8_t i = 0; i < crypto::aes_ctr_drbg_seed_size; i++) {
            tmp[i] ^= data[i];
        }

        memcpy(ctx.key.data(), tmp, crypto::aes_key_size);
        memcpy(ctx.counter.data(), tmp + crypto::aes_key_size, crypto::aes_block_size);
        return SL_STATUS_OK;
    }

    sl_status_t Crypto::aes_ctr_drbg_init(crypto::CtrDrbgContext &ctx, std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size> entropy_input, std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size> pers_string) const
    {
        crypto::CtrDrbgContext next_ctx = {};
        uint8_t seed[crypto::aes_ctr_drbg_seed_size];

        memcpy(seed, entropy_input.data(), sizeof(seed));
        for (uint8_t i = 0; i < crypto::aes_ctr_drbg_seed_size; i++) {
            seed[i] ^= pers_string[i];
        }

        sl_status_t status = update_ctr_drbg(next_ctx, seed);
        if (status != SL_STATUS_OK) {
            return status;
        }

        ctx = next_ctx;
        return SL_STATUS_OK;
    }

    sl_status_t Crypto::aes_ctr_drbg_generate(crypto::CtrDrbgContext &ctx, std::span<uint8_t> out) const
    {
        if (out.data() == nullptr || out.size() != crypto::aes_ctr_drbg_output_size) {
            return SL_STATUS_INVALID_PARAMETER;
        }

        crypto::CtrDrbgContext next_ctx = ctx;
        uint8_t block[crypto::aes_block_size];
        uint8_t provided_data[crypto::aes_ctr_drbg_seed_size] = {0};
        uint8_t generated[crypto::aes_ctr_drbg_output_size];
        uint8_t *generated_pos = generated;
        size_t copy;
        size_t remaining = crypto::aes_ctr_drbg_output_size;

        while (remaining != 0U) {
            sl_status_t status = increment(next_ctx.counter.data(), crypto::aes_block_size);
            if (status != SL_STATUS_OK) {
                return status;
            }
            status = aes_ecb_encrypt(std::span<const uint8_t, crypto::aes_key_size>(next_ctx.key), std::span<const uint8_t, crypto::aes_block_size>(next_ctx.counter), std::span<uint8_t, crypto::aes_block_size>(block));
            if (status != SL_STATUS_OK) {
                return status;
            }
            copy = (remaining < crypto::aes_block_size) ? remaining : crypto::aes_block_size;
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

}  // namespace crypto
