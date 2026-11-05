#include "crypto_impl.hpp"
#include "scoped_zeroize.hpp"

#include <algorithm>
#include <array>

#include "psa/crypto.h"
#include "psa/util.hpp"
#include "psa/psa_resources.hpp"

namespace
{

    constexpr std::size_t cbc_mac_chunk_size = 16 * crypto::aes_block_size;
    constexpr std::array<uint8_t, crypto::aes_block_size> cbc_mac_zero_iv {};

}  // namespace

namespace crypto
{

    sl_status_t Crypto::aes_cbc_mac(std::span<const uint8_t, crypto::aes_key_size> key, std::span<const uint8_t, crypto::aes_block_size> iv, std::span<const uint8_t> data, std::span<uint8_t, crypto::aes_block_size> mac) const
    {
        detail::ScopedKey imported_key(psa_wrappers_);
        detail::ScopedCipherOperation operation(psa_wrappers_);
        psa_status_t status = psa_wrappers_.crypto_init();
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = imported_key.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING, key, PSA_KEY_TYPE_AES, crypto::aes_key_bits);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = operation.encrypt_setup(imported_key.id(), PSA_ALG_CBC_NO_PADDING);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        status = psa_wrappers_.cipher_set_iv(operation.get(), cbc_mac_zero_iv);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        std::array<uint8_t, PSA_CIPHER_UPDATE_OUTPUT_SIZE(PSA_KEY_TYPE_AES, PSA_ALG_CBC_NO_PADDING, cbc_mac_chunk_size)> ciphertext {};
        const common::ScopedZeroize wipe_ciphertext(ciphertext.data(), sizeof(ciphertext));
        std::array<uint8_t, crypto::aes_block_size> last_block {};
        const common::ScopedZeroize wipe_last_block(last_block.data(), sizeof(last_block));
        std::size_t pending_blocks = 0;

        const auto retain_output = [&](std::size_t length) {
            if (length > ciphertext.size() || length % crypto::aes_block_size != 0 || length / crypto::aes_block_size > pending_blocks) {
                return false;
            }
            pending_blocks -= length / crypto::aes_block_size;
            if (length != 0) {
                std::copy_n(ciphertext.begin() + length - crypto::aes_block_size, last_block.size(), last_block.begin());
            }
            return true;
        };

        const auto update = [&](std::span<const uint8_t> input) {
            pending_blocks += input.size() / crypto::aes_block_size;
            std::size_t length = 0;
            const auto result  = psa_wrappers_.cipher_update(operation.get(), input, ciphertext, length);
            if (result != PSA_SUCCESS) {
                return psa_status_to_sl_status(result);
            }
            return retain_output(length) ? SL_STATUS_OK : SL_STATUS_FAIL;
        };

        auto result = update(iv);
        if (result != SL_STATUS_OK) {
            return result;
        }

        while (data.size() >= crypto::aes_block_size) {
            const auto length = std::min(data.size() - (data.size() % crypto::aes_block_size), cbc_mac_chunk_size);
            result            = update(data.first(length));
            if (result != SL_STATUS_OK) {
                return result;
            }
            data = data.subspan(length);
        }

        if (!data.empty()) {
            std::array<uint8_t, crypto::aes_block_size> padded {};
            const common::ScopedZeroize wipe_padded(padded.data(), sizeof(padded));
            std::copy(data.begin(), data.end(), padded.begin());
            result = update(padded);
            if (result != SL_STATUS_OK) {
                return result;
            }
        }

        std::size_t finish_len = 0;
        status                 = operation.finish(ciphertext, finish_len);
        if (status != PSA_SUCCESS) {
            return psa_status_to_sl_status(status);
        }

        if (!retain_output(finish_len) || pending_blocks != 0) {
            return SL_STATUS_FAIL;
        }
        std::copy(last_block.begin(), last_block.end(), mac.begin());
        return SL_STATUS_OK;
    }

}  // namespace crypto
