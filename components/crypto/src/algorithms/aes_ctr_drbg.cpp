#include <algorithm>
#include <array>
#include <cstdint>

#include "crypto_impl.hpp"
#include "scoped_zeroize.hpp"

namespace
{

    void increment(std::span<uint8_t, crypto::aes_block_size> counter) noexcept
    {
        for (auto byte = counter.rbegin(); byte != counter.rend(); ++byte) {
            ++(*byte);
            if (*byte != 0U) {
                break;
            }
        }
    }

}  // namespace

namespace crypto
{

    sl_status_t Crypto::update_ctr_drbg(crypto::CtrDrbgContext &ctx, std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size> data) const
    {
        std::array<uint8_t, crypto::aes_ctr_drbg_seed_size> tmp {};
        const common::ScopedZeroize wipe_tmp(tmp.data(), sizeof(tmp));

        for (std::size_t offset = 0; offset < tmp.size(); offset += crypto::aes_block_size) {
            increment(ctx.counter);
            const auto block         = std::span(tmp).subspan(offset, crypto::aes_block_size);
            const sl_status_t status = aes_ecb_encrypt(ctx.key, ctx.counter, std::span<uint8_t, crypto::aes_block_size>(block));
            if (status != SL_STATUS_OK) {
                return status;
            }
        }

        for (std::size_t i = 0; i < tmp.size(); ++i) {
            tmp[i] ^= data[i];
        }

        std::copy_n(tmp.begin(), ctx.key.size(), ctx.key.begin());
        std::copy_n(tmp.begin() + crypto::aes_key_size, ctx.counter.size(), ctx.counter.begin());
        return SL_STATUS_OK;
    }

    sl_status_t Crypto::aes_ctr_drbg_init(crypto::CtrDrbgContext &ctx, std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size> entropy_input, std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size> pers_string) const
    {
        crypto::CtrDrbgContext next_ctx = {};
        const common::ScopedZeroize wipe_context(&next_ctx, sizeof(next_ctx));
        std::array<uint8_t, crypto::aes_ctr_drbg_seed_size> seed {};
        const common::ScopedZeroize wipe_seed(seed.data(), sizeof(seed));

        for (std::size_t i = 0; i < seed.size(); ++i) {
            seed[i] = entropy_input[i] ^ pers_string[i];
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
        const common::ScopedZeroize wipe_context(&next_ctx, sizeof(next_ctx));
        static_assert(crypto::aes_ctr_drbg_output_size == crypto::aes_block_size);
        std::array<uint8_t, crypto::aes_ctr_drbg_output_size> generated {};
        const common::ScopedZeroize wipe_generated(generated.data(), sizeof(generated));
        const std::array<uint8_t, crypto::aes_ctr_drbg_seed_size> provided_data {};

        increment(next_ctx.counter);
        sl_status_t status = aes_ecb_encrypt(next_ctx.key, next_ctx.counter, generated);
        if (status != SL_STATUS_OK) {
            return status;
        }

        status = update_ctr_drbg(next_ctx, provided_data);
        if (status != SL_STATUS_OK) {
            return status;
        }

        ctx = next_ctx;
        std::copy(generated.begin(), generated.end(), out.begin());
        return SL_STATUS_OK;
    }

}  // namespace crypto
