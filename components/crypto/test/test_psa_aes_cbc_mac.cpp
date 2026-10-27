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

#include <array>
#include <climits>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "aes_cbc_mac.hpp"
#include "aes_ecb.h"
#include "test_fixture.hpp"

/* NIST SP 800-38A AES-128 key and IV (same material as OFB/CMAC tests). */

static const uint8_t nist_key_128[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
static const uint8_t nist_iv[]      = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

static const uint8_t nist_plain_16[] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};

/*
 * Z-Wave S0 auth-data layout (IV passed separately to crypto_aes_cbc_mac):
 * command, sender node ID, receiver node ID, payload length, encrypted payload.
 */
static const uint8_t s0_auth_iv[]   = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
static const uint8_t s0_auth_data[] = {
  0x81, /* SECURITY_MESSAGE_ENCAPSULATION */
  0x03, /* sender */
  0x04, /* receiver */
  0x02, /* encrypted payload length */
  0x11,
  0x22, /* encrypted payload (example ciphertext) */
};

namespace
{
    void ExpectBytesEq(const uint8_t *expected, const uint8_t *actual, size_t len)
    {
        EXPECT_THAT(std::vector<uint8_t>(actual, actual + len), ::testing::ElementsAreArray(expected, len));
    }

    void AssertS0CbcMac(const uint8_t *key, const uint8_t *iv, const uint8_t *message, uint8_t message_len, const uint8_t *expected_mac)
    {
        uint8_t calculated_mac[16];

        ASSERT_EQ(SL_STATUS_OK, crypto_aes_cbc_mac(key, iv, message, message_len, calculated_mac));
        ExpectBytesEq(expected_mac, calculated_mac, 16);
    }

    void ComputeS0CbcMacReference(const uint8_t *key, const uint8_t *iv, const uint8_t *data, uint8_t data_len, uint8_t *mac_out)
    {
        uint8_t message[((CRYPTO_AES_BLOCK_SIZE + UINT8_MAX + CRYPTO_AES_BLOCK_SIZE - 1) / CRYPTO_AES_BLOCK_SIZE) * CRYPTO_AES_BLOCK_SIZE];
        size_t message_len = CRYPTO_AES_BLOCK_SIZE + data_len;
        uint8_t chain[CRYPTO_AES_BLOCK_SIZE] {};

        memcpy(message, iv, CRYPTO_AES_BLOCK_SIZE);
        if (data_len > 0) {
            memcpy(message + CRYPTO_AES_BLOCK_SIZE, data, data_len);
        }
        const size_t pad_len = (CRYPTO_AES_BLOCK_SIZE - (message_len % CRYPTO_AES_BLOCK_SIZE)) % CRYPTO_AES_BLOCK_SIZE;
        memset(message + message_len, 0, pad_len);
        message_len += pad_len;

        for (size_t off = 0; off < message_len; off += CRYPTO_AES_BLOCK_SIZE) {
            uint8_t input[CRYPTO_AES_BLOCK_SIZE];
            for (size_t i = 0; i < CRYPTO_AES_BLOCK_SIZE; i++) {
                input[i] = static_cast<uint8_t>(message[off + i] ^ chain[i]);
            }
            ASSERT_EQ(SL_STATUS_OK, crypto_aes_ecb_encrypt(key, input, chain));
        }
        memcpy(mac_out, chain, CRYPTO_AES_BLOCK_SIZE);
    }
}  // namespace

// Empty auth payload: MAC state is only E_K(IV) (NIST AES-128-ECB encrypt IV).
TEST_F(Crypto, S0CbcMacEmptyMessageEqualsAesEcbOfIv)
{
    const uint8_t expected_mac[] = {0x50, 0xfe, 0x67, 0xcc, 0x99, 0x6d, 0x32, 0xb6, 0xda, 0x09, 0x37, 0xe9, 0x9b, 0xaf, 0xec, 0x60};
    uint8_t ecb_out[16];

    AssertS0CbcMac(nist_key_128, nist_iv, nullptr, 0, expected_mac);
    ASSERT_EQ(SL_STATUS_OK, crypto_aes_ecb_encrypt(nist_key_128, nist_iv, ecb_out));
    ExpectBytesEq(expected_mac, ecb_out, 16);
}

// One full 16-byte block (NIST SP 800-38A plaintext); exercises one mid-loop ECB step.
TEST_F(Crypto, S0CbcMacNistPlaintextOneBlock)
{
    const uint8_t expected_mac[] = {0x66, 0x8b, 0xcf, 0x60, 0xbe, 0xb0, 0x05, 0xa3, 0x53, 0x54, 0xa2, 0x01, 0xda, 0xb3, 0x6b, 0xda};

    AssertS0CbcMac(nist_key_128, nist_iv, nist_plain_16, sizeof(nist_plain_16), expected_mac);
}

// Partial final block: spec padding path (final AES encrypt when len % 16 != 0).
TEST_F(Crypto, S0CbcMacNistPlaintextSevenBytes)
{
    const uint8_t expected_mac[] = {0x13, 0x73, 0x94, 0x01, 0x8c, 0xdf, 0xe1, 0x7f, 0x59, 0xf9, 0x77, 0x97, 0x9e, 0xff, 0x71, 0x08};

    AssertS0CbcMac(nist_key_128, nist_iv, nist_plain_16, 7, expected_mac);
}

// Two AES blocks: second block triggers another ECB iteration at byte index 15.
TEST_F(Crypto, S0CbcMacNistPlaintextTwoBlocks)
{
    uint8_t message[32];
    const uint8_t expected_mac[] = {0x11, 0xe0, 0xad, 0xbc, 0x92, 0x31, 0xe1, 0x3d, 0xd8, 0x43, 0x6e, 0x69, 0xd9, 0xf3, 0x14, 0xcc};

    memcpy(message, nist_plain_16, 16);
    memcpy(message + 16, nist_plain_16, 16);

    AssertS0CbcMac(nist_key_128, nist_iv, message, sizeof(message), expected_mac);
}

// Typical S0 authentication data size (header + short encrypted payload).
TEST_F(Crypto, S0CbcMacMessageEncapsulationAuthData)
{
    const uint8_t expected_mac[] = {0x4c, 0x5c, 0xb1, 0x62, 0xa5, 0x17, 0x4c, 0x77, 0xf4, 0x11, 0x84, 0xa2, 0x5a, 0xe7, 0xb6, 0x0c};

    AssertS0CbcMac(nist_key_128, s0_auth_iv, s0_auth_data, sizeof(s0_auth_data), expected_mac);
}

// C++ CBC-MAC API matches the S0 encapsulation auth-data vector.
TEST_F(Crypto, AesCbcMacCppApi)
{
    std::array<uint8_t, CRYPTO_AES_KEY_SIZE> key {};
    memcpy(key.data(), nist_key_128, sizeof(key));
    std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> iv {};
    memcpy(iv.data(), s0_auth_iv, sizeof(iv));
    const uint8_t expected_mac[] = {0x4c, 0x5c, 0xb1, 0x62, 0xa5, 0x17, 0x4c, 0x77, 0xf4, 0x11, 0x84, 0xa2, 0x5a, 0xe7, 0xb6, 0x0c};
    std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> mac {};

    ASSERT_EQ(SL_STATUS_OK, crypto::aes_cbc_mac(key, iv, std::span<const uint8_t>(s0_auth_data, sizeof(s0_auth_data)), mac));
    ExpectBytesEq(expected_mac, mac.data(), mac.size());
}

// C++ API rejects message lengths that exceed the C API uint8_t limit.
TEST_F(Crypto, AesCbcMacCppApiRejectsOversizedData)
{
    std::array<uint8_t, CRYPTO_AES_KEY_SIZE> key {};
    std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> iv {};
    std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> mac {};
    std::vector<uint8_t> oversized(static_cast<size_t>(UINT8_MAX) + 1U);

    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, crypto::aes_cbc_mac(key, iv, oversized, mac));
}

// Maximum C API payload length (uint8_t): padded IV||data must fit internal buffer.
TEST_F(Crypto, S0CbcMacMaxPayloadLength255)
{
    uint8_t data[UINT8_MAX];
    uint8_t expected_mac[CRYPTO_AES_BLOCK_SIZE];
    uint8_t calculated_mac[CRYPTO_AES_BLOCK_SIZE];

    for (size_t i = 0; i < UINT8_MAX; i++) {
        data[i] = static_cast<uint8_t>(i);
    }
    ComputeS0CbcMacReference(nist_key_128, nist_iv, data, UINT8_MAX, expected_mac);

    ASSERT_EQ(SL_STATUS_OK, crypto_aes_cbc_mac(nist_key_128, nist_iv, data, UINT8_MAX, calculated_mac));
    ExpectBytesEq(expected_mac, calculated_mac, CRYPTO_AES_BLOCK_SIZE);
}

// C++ CBC-MAC at max span length matches C API and ECB-chained reference.
TEST_F(Crypto, AesCbcMacCppApiMaxPayloadLength255)
{
    std::array<uint8_t, CRYPTO_AES_KEY_SIZE> key {};
    memcpy(key.data(), nist_key_128, sizeof(key));
    std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> iv {};
    memcpy(iv.data(), nist_iv, sizeof(iv));
    std::array<uint8_t, UINT8_MAX> data {};
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<uint8_t>(i);
    }
    uint8_t expected_mac[CRYPTO_AES_BLOCK_SIZE];
    std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> mac_cpp {};
    uint8_t calculated_mac[CRYPTO_AES_BLOCK_SIZE];

    ComputeS0CbcMacReference(nist_key_128, nist_iv, data.data(), UINT8_MAX, expected_mac);
    ASSERT_EQ(SL_STATUS_OK, crypto_aes_cbc_mac(nist_key_128, nist_iv, data.data(), UINT8_MAX, calculated_mac));
    ASSERT_EQ(SL_STATUS_OK, crypto::aes_cbc_mac(key, iv, std::span<const uint8_t>(data.data(), data.size()), mac_cpp));

    ExpectBytesEq(expected_mac, calculated_mac, CRYPTO_AES_BLOCK_SIZE);
    ExpectBytesEq(expected_mac, mac_cpp.data(), mac_cpp.size());
    ExpectBytesEq(calculated_mac, mac_cpp.data(), mac_cpp.size());
}
