#include "crypto.h"

#include <algorithm>

#include "c_api_backend.hpp"
#include "secure_memzero.h"

static_assert(CRYPTO_AES_KEY_BITS == crypto::aes_key_bits);
static_assert(CRYPTO_AES_KEY_SIZE == crypto::aes_key_size);
static_assert(CRYPTO_AES_BLOCK_SIZE == crypto::aes_block_size);
static_assert(CRYPTO_AES_CMAC_OUTPUT_LEN == crypto::aes_cmac_output_len);
static_assert(CRYPTO_AES_CTR_DRBG_SEED_SIZE == crypto::aes_ctr_drbg_seed_size);
static_assert(CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE == crypto::aes_ctr_drbg_output_size);
static_assert(CRYPTO_X25519_KEY_BITS == crypto::x25519_key_bits);
static_assert(CRYPTO_X25519_PRIVATE_KEY_SIZE == crypto::x25519_private_key_size);
static_assert(CRYPTO_X25519_PUBLIC_KEY_SIZE == crypto::x25519_public_key_size);
static_assert(CRYPTO_X25519_SHARED_SECRET_SIZE == crypto::x25519_shared_secret_size);

namespace
{
    void copy_context(const crypto::CtrDrbgContext &source, crypto_aes_ctr_drbg_ctx_t &destination)
    {
        std::copy(source.counter.begin(), source.counter.end(), destination.counter);
        std::copy(source.key.begin(), source.key.end(), destination.key);
    }
}  // namespace

extern "C" {

sl_status_t crypto_aes_ecb_encrypt(const uint8_t *key, const uint8_t *input, uint8_t *output)
{
    if (key == nullptr || input == nullptr || output == nullptr) {
        return SL_STATUS_NULL_POINTER;
    }

    return crypto_c::detail::backend().aes_ecb_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE), std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>(input, CRYPTO_AES_BLOCK_SIZE), std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE>(output, CRYPTO_AES_BLOCK_SIZE));
}

sl_status_t crypto_aes_ccm_encrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, const uint8_t *plaintext, size_t plaintext_len, uint8_t *ciphertext, size_t ciphertext_size, size_t *ciphertext_len)
{
    if (key == nullptr || ciphertext_len == nullptr || (nonce == nullptr && nonce_len != 0) || (aad == nullptr && aad_len != 0) || (plaintext == nullptr && plaintext_len != 0) || (ciphertext == nullptr && ciphertext_size != 0)) {
        return SL_STATUS_NULL_POINTER;
    }

    return crypto_c::detail::backend().aes_ccm_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE),
                                                       tag_len,
                                                       std::span<const uint8_t>(nonce, nonce_len),
                                                       std::span<const uint8_t>(aad, aad_len),
                                                       std::span<const uint8_t>(plaintext, plaintext_len),
                                                       std::span<uint8_t>(ciphertext, ciphertext_size),
                                                       *ciphertext_len);
}

sl_status_t crypto_aes_ccm_decrypt(const uint8_t *key, size_t tag_len, const uint8_t *nonce, size_t nonce_len, const uint8_t *aad, size_t aad_len, const uint8_t *ciphertext, size_t ciphertext_len, uint8_t *plaintext, size_t plaintext_size, size_t *plaintext_len)
{
    if (key == nullptr || plaintext_len == nullptr || (nonce == nullptr && nonce_len != 0) || (aad == nullptr && aad_len != 0) || (ciphertext == nullptr && ciphertext_len != 0) || (plaintext == nullptr && plaintext_size != 0)) {
        return SL_STATUS_NULL_POINTER;
    }

    return crypto_c::detail::backend().aes_ccm_decrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE),
                                                       tag_len,
                                                       std::span<const uint8_t>(nonce, nonce_len),
                                                       std::span<const uint8_t>(aad, aad_len),
                                                       std::span<const uint8_t>(ciphertext, ciphertext_len),
                                                       std::span<uint8_t>(plaintext, plaintext_size),
                                                       *plaintext_len);
}

sl_status_t crypto_aes_cmac_compute(const uint8_t *key, const uint8_t *message, size_t message_length, uint8_t *mac)
{
    if (key == nullptr || mac == nullptr || (message == nullptr && message_length != 0)) {
        return SL_STATUS_NULL_POINTER;
    }

    return crypto_c::detail::backend().aes_cmac_compute(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE), std::span<const uint8_t>(message, message_length), std::span<uint8_t, CRYPTO_AES_CMAC_OUTPUT_LEN>(mac, CRYPTO_AES_CMAC_OUTPUT_LEN));
}

sl_status_t crypto_aes_cbc_mac(const uint8_t *key, const uint8_t *iv, const uint8_t *data, uint8_t len, uint8_t *mac)
{
    if (key == nullptr || iv == nullptr || mac == nullptr || (data == nullptr && len != 0)) {
        return SL_STATUS_NULL_POINTER;
    }

    return crypto_c::detail::backend().aes_cbc_mac(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE),
                                                   std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>(iv, CRYPTO_AES_BLOCK_SIZE),
                                                   std::span<const uint8_t>(data, len),
                                                   std::span<uint8_t, CRYPTO_AES_BLOCK_SIZE>(mac, CRYPTO_AES_BLOCK_SIZE));
}

sl_status_t crypto_aes_ofb_xor(const uint8_t *key, uint8_t iv[16], uint8_t *data, size_t len)
{
    if (len == 0) {
        return SL_STATUS_OK;
    }

    if (key == nullptr || iv == nullptr || data == nullptr) {
        return SL_STATUS_NULL_POINTER;
    }

    return crypto_c::detail::backend().aes_ofb_xor(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE>(key, CRYPTO_AES_KEY_SIZE), std::span<const uint8_t, CRYPTO_AES_BLOCK_SIZE>(iv, CRYPTO_AES_BLOCK_SIZE), std::span<uint8_t>(data, len));
}

sl_status_t crypto_aes_ctr_drbg_init(crypto_aes_ctr_drbg_ctx_t *ctx, const uint8_t *entropy_input, const uint8_t *pers_string)
{
    if (ctx == nullptr || entropy_input == nullptr || pers_string == nullptr) {
        return SL_STATUS_NULL_POINTER;
    }

    crypto::CtrDrbgContext next_ctx {};
    const auto status = crypto_c::detail::backend().aes_ctr_drbg_init(next_ctx, std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE>(entropy_input, CRYPTO_AES_CTR_DRBG_SEED_SIZE), std::span<const uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE>(pers_string, CRYPTO_AES_CTR_DRBG_SEED_SIZE));
    if (status == SL_STATUS_OK) {
        copy_context(next_ctx, *ctx);
    }
    secure_memzero(&next_ctx, sizeof(next_ctx));
    return status;
}

sl_status_t crypto_aes_ctr_drbg_generate(crypto_aes_ctr_drbg_ctx_t *ctx, uint8_t *out)
{
    if (ctx == nullptr || out == nullptr) {
        return SL_STATUS_NULL_POINTER;
    }

    crypto::CtrDrbgContext next_ctx;
    std::copy_n(ctx->counter, next_ctx.counter.size(), next_ctx.counter.begin());
    std::copy_n(ctx->key, next_ctx.key.size(), next_ctx.key.begin());
    std::array<uint8_t, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE> generated;
    const auto status = crypto_c::detail::backend().aes_ctr_drbg_generate(next_ctx, generated);
    if (status == SL_STATUS_OK) {
        // Match the core's commit order, including output that overlaps ctx.
        copy_context(next_ctx, *ctx);
        std::copy(generated.begin(), generated.end(), out);
    }
    secure_memzero(generated.data(), sizeof(generated));
    secure_memzero(&next_ctx, sizeof(next_ctx));
    return status;
}

sl_status_t crypto_x25519_derive_public_key(const uint8_t *private_key, uint8_t *public_key)
{
    if (private_key == nullptr || public_key == nullptr) {
        return SL_STATUS_NULL_POINTER;
    }

    return crypto_c::detail::backend().x25519_derive_public_key(std::span<const uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE>(private_key, CRYPTO_X25519_PRIVATE_KEY_SIZE), std::span<uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE>(public_key, CRYPTO_X25519_PUBLIC_KEY_SIZE));
}

sl_status_t crypto_x25519_derive_shared_secret(const uint8_t *private_key, const uint8_t *peer_public_key, uint8_t *shared_secret)
{
    if (private_key == nullptr || peer_public_key == nullptr || shared_secret == nullptr) {
        return SL_STATUS_NULL_POINTER;
    }

    return crypto_c::detail::backend().x25519_derive_shared_secret(std::span<const uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE>(private_key, CRYPTO_X25519_PRIVATE_KEY_SIZE),
                                                                   std::span<const uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE>(peer_public_key, CRYPTO_X25519_PUBLIC_KEY_SIZE),
                                                                   std::span<uint8_t, CRYPTO_X25519_SHARED_SECRET_SIZE>(shared_secret, CRYPTO_X25519_SHARED_SECRET_SIZE));
}

sl_status_t crypto_generate_random(uint8_t *output, size_t output_size)
{
    if (output == nullptr && output_size != 0) {
        return SL_STATUS_NULL_POINTER;
    }

    return crypto_c::detail::backend().generate_random(std::span<uint8_t>(output, output_size));
}

}  // extern "C"
