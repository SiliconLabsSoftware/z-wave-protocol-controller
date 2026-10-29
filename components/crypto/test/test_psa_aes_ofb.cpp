/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "aes_ofb.hpp"
#include "test_fixture.hpp"

/* NIST SP 800-38A Appendix F.4.1 (OFB-AES128.Encrypt) */

static const uint8_t nist_key_128[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
static const uint8_t nist_iv[]      = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

static const uint8_t nist_plain_64[] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a, 0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
                                        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef, 0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10};

static const uint8_t nist_cipher_64[] = {0x3b, 0x3f, 0xd9, 0x2e, 0xb7, 0x2d, 0xad, 0x20, 0x33, 0x34, 0x49, 0xf8, 0xe8, 0x3c, 0xfb, 0x4a, 0x77, 0x89, 0x50, 0x8d, 0x16, 0x91, 0x8f, 0x03, 0xf5, 0x3c, 0x52, 0xda, 0xc5, 0x4e, 0xd8, 0x25,
                                         0x97, 0x40, 0x05, 0x1e, 0x9c, 0x5f, 0xec, 0xf6, 0x43, 0x44, 0xf7, 0xa8, 0x22, 0x60, 0xed, 0xcc, 0x30, 0x4c, 0x65, 0x28, 0xf6, 0x59, 0xc7, 0x78, 0x66, 0xa5, 0x10, 0xd9, 0xc1, 0xd6, 0xae, 0x5e};

namespace
{
    void ExpectBytesEq(const uint8_t *expected, const uint8_t *actual, size_t len)
    {
        EXPECT_THAT(std::vector<uint8_t>(actual, actual + len), ::testing::ElementsAreArray(expected, len));
    }
}  // namespace

// NIST OFB-AES128 encrypt over one 16-byte block.
TEST_F(Crypto, NistSp80038aOfbAes128EncryptOneBlock)
{
    uint8_t iv[16];
    uint8_t buffer[16];

    memcpy(iv, nist_iv, sizeof(iv));
    memcpy(buffer, nist_plain_64, sizeof(buffer));

    ASSERT_EQ(SL_STATUS_OK, crypto_aes_ofb_xor(nist_key_128, iv, buffer, sizeof(buffer)));
    ExpectBytesEq(nist_cipher_64, buffer, sizeof(buffer));
}

// NIST OFB-AES128 encrypt over 64 bytes.
TEST_F(Crypto, NistSp80038aOfbAes128Encrypt64Bytes)
{
    uint8_t iv[16];
    uint8_t buffer[64];

    memcpy(iv, nist_iv, sizeof(iv));
    memcpy(buffer, nist_plain_64, sizeof(buffer));

    ASSERT_EQ(SL_STATUS_OK, crypto_aes_ofb_xor(nist_key_128, iv, buffer, sizeof(buffer)));
    ExpectBytesEq(nist_cipher_64, buffer, sizeof(buffer));
}

// NIST OFB-AES128 encrypt over a 7-byte partial block.
TEST_F(Crypto, NistSp80038aOfbAes128EncryptPartialSevenBytes)
{
    uint8_t iv[16];
    uint8_t buffer[7];

    memcpy(iv, nist_iv, sizeof(iv));
    memcpy(buffer, nist_plain_64, sizeof(buffer));

    ASSERT_EQ(SL_STATUS_OK, crypto_aes_ofb_xor(nist_key_128, iv, buffer, sizeof(buffer)));
    ExpectBytesEq(nist_cipher_64, buffer, sizeof(buffer));
}

// OFB decrypt is XOR with the same keystream; ciphertext round-trips to plaintext.
TEST_F(Crypto, NistSp80038aOfbAes128DecryptViaXor)
{
    uint8_t iv[16];
    uint8_t buffer[64];

    memcpy(iv, nist_iv, sizeof(iv));
    memcpy(buffer, nist_cipher_64, sizeof(buffer));

    ASSERT_EQ(SL_STATUS_OK, crypto_aes_ofb_xor(nist_key_128, iv, buffer, sizeof(buffer)));
    ExpectBytesEq(nist_plain_64, buffer, sizeof(buffer));
}

// Zero-length OFB leaves IV and data unchanged.
TEST_F(Crypto, OfbAes128ZeroLengthLeavesIvAndDataUnchanged)
{
    uint8_t iv[16];
    const uint8_t data_expected[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t data[4];

    memcpy(iv, nist_iv, sizeof(iv));
    memcpy(data, data_expected, sizeof(data));

    ASSERT_EQ(SL_STATUS_OK, crypto_aes_ofb_xor(nist_key_128, iv, data, 0));
    ExpectBytesEq(nist_iv, iv, sizeof(iv));
    ExpectBytesEq(data_expected, data, sizeof(data));
}

// C++ span OFB API matches NIST one-block encrypt vector.
TEST_F(Crypto, AesOfbXorCppApi)
{
    std::array<uint8_t, CRYPTO_AES_KEY_SIZE> key {};
    memcpy(key.data(), nist_key_128, sizeof(key));
    std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> iv {};
    memcpy(iv.data(), nist_iv, sizeof(iv));
    std::array<uint8_t, 16> buffer {};
    memcpy(buffer.data(), nist_plain_64, buffer.size());

    ASSERT_EQ(SL_STATUS_OK, crypto::aes_ofb_xor(key, iv, std::span<uint8_t>(buffer.data(), buffer.size())));
    ExpectBytesEq(nist_cipher_64, buffer.data(), buffer.size());
}

// Zero-length C API with null pointers returns OK without touching spans.
TEST_F(Crypto, OfbAes128ZeroLengthNullPointersOk)
{
    EXPECT_EQ(SL_STATUS_OK, crypto_aes_ofb_xor(nullptr, nullptr, nullptr, 0));
}

TEST_F(Crypto, OfbCppEmptyDataPreservesKeyAndIv)
{
    std::array<uint8_t, CRYPTO_AES_KEY_SIZE> key;
    std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> iv;
    std::copy_n(nist_key_128, key.size(), key.begin());
    std::copy_n(nist_iv, iv.size(), iv.begin());
    const auto saved_key = key;
    const auto saved_iv  = iv;
    EXPECT_EQ(SL_STATUS_OK, crypto::aes_ofb_xor(key, iv, {}));
    EXPECT_EQ(saved_key, key);
    EXPECT_EQ(saved_iv, iv);
}
