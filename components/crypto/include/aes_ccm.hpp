#ifndef AES_CCM_HPP
#define AES_CCM_HPP

#include "aes_ccm.h"

#include <span>

#include "aes.h"

namespace crypto
{

    sl_status_t aes_ccm_encrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, size_t tag_len, std::span<const uint8_t> nonce, std::span<const uint8_t> aad, std::span<const uint8_t> plaintext, std::span<uint8_t> ciphertext, size_t &ciphertext_len);

    sl_status_t aes_ccm_decrypt(std::span<const uint8_t, CRYPTO_AES_KEY_SIZE> key, size_t tag_len, std::span<const uint8_t> nonce, std::span<const uint8_t> aad, std::span<const uint8_t> ciphertext, std::span<uint8_t> plaintext, size_t &plaintext_len);

}  // namespace crypto

#endif
